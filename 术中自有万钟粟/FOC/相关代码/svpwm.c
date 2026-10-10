/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file svpwm.c
 *
 * @par dependencies
 * - stdint.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief SVPWM算法: 把反Park输出的 Uα/Uβ 合成三相PWM占空比(CCR值).
 *
 * ============================================================================
 *                              调用方式
 * ============================================================================
 *
 *   前提: 定时器必须配成 "中心对齐(CMS=up/down) + PWM Mode 2 + 高电平有效",
 *         这样 CCR = ARR * (1 - D), 本文件内部按该模型推导.
 *
 *   // 1. 在控制中断(通常是ADC采样完成/PWM下溢中断)里准备入参
 *   svpwm_input_t  in  = {
 *       .u_alpha =  ualpha_ref,   // 反Park给出的α轴目标电压 (V)
 *       .u_beta  =  ubeta_ref,    // 反Park给出的β轴目标电压 (V)
 *       .v_dc    =  vdc_measured, // 实测母线电压 (V)
 *       .t_pwm   =  50e-6f,       // PWM周期 (s), 20kHz=50us
 *       .k0      =  0.5f,         // 零矢量分配系数:
 *                                 //   0.5 → 对称7段式(最平滑,推荐)
 *                                 //   0.0 → 5段式(贴地,DPWMMAX)
 *                                 //   1.0 → 5段式(贴顶,DPWMMIN)
 *       .arr     =  2000U,        // 定时器ARR寄存器值
 *   };
 *
 *   // 2. 调用一次就得到三个通道的CCR
 *   svpwm_output_t out;
 *   if (SVPWM_OK == svpwm_calc(&in, &out)) {
 *       __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, out.ccr_u);
 *       __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, out.ccr_v);
 *       __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, out.ccr_w);
 *   }
 *
 *   // out.sector 是本次所在扇区(1~6), 调试/示波器触发时很有用.
 *
 *   注意事项:
 *     * 当 |Uref| > Vdc/√3 时会触发过调制, 内部会按 T1+T2≤T 自动缩放,
 *       避免CCR溢出, 但代价是输出电压失真, 应在上层做电压限幅.
 *     * v_dc 和 t_pwm 不能为0(内部有除法), 否则返回 SVPWM_ERR_PARAM.
 *
 * ============================================================================
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes***********************************//

#include <stddef.h>
#include <stdint.h>

//********************************Includes***********************************//


//*********************************Defines***********************************//

#define SVPWM_SQRT3         (1.7320508075688772f)  /* √3           */
#define SVPWM_SQRT3_HALF    (0.8660254037844386f)  /* √3 / 2       */
#define SVPWM_HALF          (0.5f)                 /* 1 / 2        */

//*********************************Defines***********************************//


//*******************************Enumerations********************************//

typedef enum
{
    SVPWM_OK            = 0,    /* 计算成功           */
    SVPWM_ERR_NULL      = 1,    /* 空指针             */
    SVPWM_ERR_PARAM     = 2     /* Vdc或Tpwm非法      */
} svpwm_status_t;

//*******************************Enumerations********************************//


//*********************************Structs***********************************//

typedef struct
{
    float    u_alpha;   /* α轴目标电压 (V)              */
    float    u_beta;    /* β轴目标电压 (V)              */
    float    v_dc;      /* 母线电压 (V), 必须>0         */
    float    t_pwm;     /* PWM周期 (s), 必须>0          */
    float    k0;        /* 零矢量系数 [0,1], 0.5=7段式  */
    uint16_t arr;       /* 定时器ARR值                   */
} svpwm_input_t;

typedef struct
{
    uint16_t ccr_u;     /* U相CCR, 直接写TIMx->CCR1     */
    uint16_t ccr_v;     /* V相CCR, 直接写TIMx->CCR2     */
    uint16_t ccr_w;     /* W相CCR, 直接写TIMx->CCR3     */
    uint8_t  sector;    /* 本次所在扇区1~6, 调试用       */
} svpwm_output_t;

//*********************************Structs***********************************//


/**
 * @brief 把浮点Tcmx限幅并截断成uint16_t的CCR值.
 *
 * @param[in] tcm_sec : Tcmx (秒).
 * @param[in] scale   : 换算系数 = 2*ARR / T.
 * @param[in] arr_max : CCR上限 = ARR.
 *
 * @return uint16_t : 夹紧后的CCR值.
 *
 */
static uint16_t svpwm_tcm_to_ccr(
    const float    tcm_sec,
    const float      scale,
    const uint16_t arr_max )
{
    float    val;
    uint16_t ccr;

    val = tcm_sec * scale;

    /* 负值夹到0, 超量程夹到ARR */
    if (val <= 0.0f)
    {
        ccr = 0U;
    }
    else if (val >= (float)arr_max)
    {
        ccr = arr_max;
    }
    else
    {
        ccr = (uint16_t)val;
    }

    return ccr;
}


/**
 * @brief SVPWM计算: Uα, Uβ → 三相CCR值.
 *
 * Steps:
 *  1. 参数校验(空指针/Vdc/Tpwm).
 *  2. 扇区判断: A=Uβ, B=√3/2·Uα-1/2·Uβ, C=-√3/2·Uα-1/2·Uβ,
 *               N = sign(A)+2·sign(B)+4·sign(C), 再N→扇区.
 *  3. 算时间辅助量 X/Y/Z, K = √3·T/Vdc.
 *  4. 按扇区查表选 T1, T2.
 *  5. 过调制钳位: 若T1+T2>T则按比例缩放,保证T0≥0.
 *  6. 排时序: Tcm1 = (1-K0)·T0/2, Tcm2 = Tcm1 + T1/2, Tcm3 = Tcm2 + T2/2.
 *  7. 按扇区把Tcm1/Tcm2/Tcm3分配到U/V/W三相.
 *  8. 换算成CCR: CCR = 2·ARR·Tcmx / T (中心对齐+Mode2+高有效模型).
 *
 * @param[in]  p_in  : 输入参数指针.
 * @param[out] p_out : 输出结果指针.
 *
 * @return svpwm_status_t : 函数执行状态.
 *
 */
svpwm_status_t svpwm_calc(
    const svpwm_input_t * const  p_in,
    svpwm_output_t      * const p_out )
{
    float    a_val;
    float    b_val;
    float    c_val;
    uint8_t  n_code;
    uint8_t  sector;
    float    k_factor;
    float    x_val;
    float    y_val;
    float    z_val;
    float    t1;
    float    t2;
    float    t0;
    float    sum;
    float    tcm1;
    float    tcm2;
    float    tcm3;
    float    tcm_u;
    float    tcm_v;
    float    tcm_w;
    float    ccr_scale;

    /* 1. 参数校验 */
    if ((NULL == p_in) || (NULL == p_out))
    {
        return SVPWM_ERR_NULL;
    }
    if ((p_in->v_dc <= 0.0f) || (p_in->t_pwm <= 0.0f))
    {
        return SVPWM_ERR_PARAM;
    }

    /* 2. 扇区判断 */
    a_val =  p_in->u_beta;
    b_val =  (SVPWM_SQRT3_HALF * p_in->u_alpha)
           - (SVPWM_HALF       * p_in->u_beta );
    c_val = -(SVPWM_SQRT3_HALF * p_in->u_alpha)
           - (SVPWM_HALF       * p_in->u_beta );

    n_code = ((a_val > 0.0f) ? 1U : 0U)
           + ((b_val > 0.0f) ? 2U : 0U)
           + ((c_val > 0.0f) ? 4U : 0U);

    /* N→Sector 经典映射表 */
    switch (n_code)
    {
        case 3U:  sector = 1U; break;
        case 1U:  sector = 2U; break;
        case 5U:  sector = 3U; break;
        case 4U:  sector = 4U; break;
        case 6U:  sector = 5U; break;
        case 2U:  sector = 6U; break;
        default:  sector = 1U; break;   /* Uα=Uβ=0, 任选扇区即可 */
    }
    p_out->sector = sector;

    /* 3. 算XYZ, K = √3·T/Vdc */
    k_factor = (SVPWM_SQRT3 * p_in->t_pwm) / p_in->v_dc;
    x_val    = k_factor *   p_in->u_beta;
    y_val    = k_factor * ( (SVPWM_SQRT3_HALF * p_in->u_alpha)
                          + (SVPWM_HALF       * p_in->u_beta ));
    z_val    = k_factor * (-(SVPWM_SQRT3_HALF * p_in->u_alpha)
                          + (SVPWM_HALF       * p_in->u_beta ));

    /* 4. 扇区查表选 T1, T2 */
    switch (sector)
    {
        case 1U:  t1 = -z_val;  t2 =  x_val;  break;
        case 2U:  t1 =  z_val;  t2 =  y_val;  break;
        case 3U:  t1 =  x_val;  t2 = -y_val;  break;
        case 4U:  t1 = -x_val;  t2 = -z_val;  break;
        case 5U:  t1 = -y_val;  t2 =  z_val;  break;
        case 6U:  t1 =  y_val;  t2 = -x_val;  break;
        default:  t1 =   0.0f;  t2 =   0.0f;  break;
    }

    /* 5. 过调制钳位: 保证 T1+T2 ≤ T */
    sum = t1 + t2;
    if (sum > p_in->t_pwm)
    {
        t1 = (t1 * p_in->t_pwm) / sum;
        t2 = (t2 * p_in->t_pwm) / sum;
    }
    t0 = p_in->t_pwm - t1 - t2;

    /* 6. 排时序(中心对齐左半) */
    tcm1 = (1.0f - p_in->k0) * t0 * SVPWM_HALF;   /* (1-K0)·T0/2 */
    tcm2 = tcm1 + (t1 * SVPWM_HALF);
    tcm3 = tcm2 + (t2 * SVPWM_HALF);

    /* 7. 按扇区分配U/V/W */
    switch (sector)
    {
        case 1U:  tcm_u = tcm1;  tcm_v = tcm2;  tcm_w = tcm3;  break;
        case 2U:  tcm_u = tcm2;  tcm_v = tcm1;  tcm_w = tcm3;  break;
        case 3U:  tcm_u = tcm3;  tcm_v = tcm1;  tcm_w = tcm2;  break;
        case 4U:  tcm_u = tcm3;  tcm_v = tcm2;  tcm_w = tcm1;  break;
        case 5U:  tcm_u = tcm2;  tcm_v = tcm3;  tcm_w = tcm1;  break;
        case 6U:  tcm_u = tcm1;  tcm_v = tcm3;  tcm_w = tcm2;  break;
        default:  tcm_u = 0.0f;  tcm_v = 0.0f;  tcm_w = 0.0f;  break;
    }

    /* 8. Tcmx(秒) → CCR count */
    /* 中心对齐+Mode2+高有效: CCR = 2·ARR·Tcmx/T */
    ccr_scale = (2.0f * (float)p_in->arr) / p_in->t_pwm;

    p_out->ccr_u = svpwm_tcm_to_ccr(tcm_u, ccr_scale, p_in->arr);
    p_out->ccr_v = svpwm_tcm_to_ccr(tcm_v, ccr_scale, p_in->arr);
    p_out->ccr_w = svpwm_tcm_to_ccr(tcm_w, ccr_scale, p_in->arr);

    return SVPWM_OK;
}
