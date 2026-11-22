/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file svpwm.c
 *
 * @par dependencies
 * - foc_transform.h
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
 *       .inv_v_dc = inv_vdc,      // 初始化时计算 1/Vdc (1/V)
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
 *     * inv_v_dc 和 t_pwm 必须为正数, 否则返回 SVPWM_ERR_PARAM.
 *
 * ============================================================================
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes***********************************//

#include "../inc/foc_transform.h"

#include <stddef.h>

//********************************Includes***********************************//


//*********************************Defines***********************************//

#define SVPWM_SQRT3         (1.7320508075688772f)  /* √3           */
#define SVPWM_SQRT3_HALF    (0.8660254037844386f)  /* √3 / 2       */
#define SVPWM_HALF          (0.5f)                 /* 1 / 2        */

/* ARMCC 默认偏向代码体积，显式内联三相 CCR 的小型转换函数。 */
#if defined(__CC_ARM)
#define SVPWM_INLINE __forceinline
#else
#define SVPWM_INLINE __inline
#endif

//*********************************Defines***********************************//


/**
 * @brief 把浮点比较计数限幅并截断成 uint16_t 的 CCR 值。
 *
 * @param[in] val     : 浮点比较计数。
 * @param[in] arr_max : CCR上限 = ARR.
 *
 * @return uint16_t : 夹紧后的CCR值.
 *
 */
static SVPWM_INLINE uint16_t svpwm_tcm_to_ccr(
    const float        val,
    const uint16_t arr_max )
{
    int32_t i32_ccr;

    /* k0∈[0,1] 且跨度已限幅，CCR 仅在 [0,ARR] 附近有舍入误差。
     * ARR 为 uint16_t，转 int32_t 有足够余量；先截断再整数限幅，
     * 避免三相各做两次浮点比较。负数不能直接转无符号整数。 */
    i32_ccr = (int32_t)val;
    if (i32_ccr < 0)
    {
        i32_ccr = 0;
    }
    else if (i32_ccr > (int32_t)arr_max)
    {
        i32_ccr = (int32_t)arr_max;
    }
    return (uint16_t)i32_ccr;
}


/**
 * @brief SVPWM计算: Uα, Uβ → 三相CCR值.
 *
 * Steps:
 *  1. 校验指针、初始化提供的母线倒数及 PWM 周期。
 *  2. 三相投影减去公共分量，一次排序确定极值与扇区。
 *  3. 根据三相跨度限幅，并按 k0 注入公共偏移。
 *  4. 三相 CCR 转为有符号整数后限幅；扇区仅用于调试输出。
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
    float f_a;
    float f_b;
    float f_c;
    float f_w;
    float f_max;
    float f_min;
    float f_span;
    float f_gain;
    float f_scale;
    float f_common;
    float f_arr;
    uint8_t u8_sector;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_in) || (NULL == p_out))
    {
        return SVPWM_ERR_NULL;
    }
    if ((p_in->inv_v_dc <= 0.0f) || (p_in->t_pwm <= 0.0f))
    {
        return SVPWM_ERR_PARAM;
    }
    //--------------------------参数检查 end--------------------------//

    /* 等效三相投影为 (B, 0, -A)。它与常规逆 Clarke 三相仅相差
     * 公共偏移和 √3 比例，因此相间电压、CCR 极性和相序保持不变。
     * A=Uβ，B=√3/2*Uα-Uβ/2；无需再排 T1/T2/T0 或重分配相序。 */
    f_a = p_in->u_beta;
    f_b = (SVPWM_SQRT3_HALF * p_in->u_alpha) - (SVPWM_HALF * f_a);
    f_w = -f_a;
    /* 用原 A/B/C 符号一次确定排序与扇区，保留边界的严格 > 判据。
     * C 只在 A/B 异号及零矢量路径计算，无需末尾重复比较、查表。 */
    if (f_a > 0.0f)
    {
        if (f_b > 0.0f)
        {
            f_max = f_b;
            f_min = f_w;
            u8_sector = 1U;
        }
        else
        {
            f_max = 0.0f;
            f_c = -(SVPWM_SQRT3_HALF * p_in->u_alpha) -
                (SVPWM_HALF * f_a);
            if (f_c > 0.0f)
            {
                f_min = f_b;
                u8_sector = 3U;
            }
            else
            {
                f_min = f_w;
                u8_sector = 2U;
            }
        }
    }
    else
    {
        if (f_b > 0.0f)
        {
            f_min = 0.0f;
            f_c = -(SVPWM_SQRT3_HALF * p_in->u_alpha) -
                (SVPWM_HALF * f_a);
            if (f_c > 0.0f)
            {
                f_max = f_w;
                u8_sector = 5U;
            }
            else
            {
                f_max = f_b;
                u8_sector = 6U;
            }
        }
        else
        {
            f_min = f_b;
            f_max = f_w;
            f_c = -(SVPWM_SQRT3_HALF * p_in->u_alpha) -
                (SVPWM_HALF * f_a);
            u8_sector = (f_c > 0.0f) ? 4U : 1U;
        }
    }
    p_out->sector = u8_sector;

    f_gain = SVPWM_SQRT3 * p_in->inv_v_dc;
    f_span = (f_max - f_min) * f_gain;
    if (f_span > 1.0f)
    {
        /* 等效于 T1/T2 同比例缩小；只需一次除法，零矢量为零。 */
        f_gain = f_gain / f_span;
        f_span = 1.0f;
    }
    f_arr = (float)p_in->arr;
    f_scale = f_arr * f_gain;
    /* CCR_i = ARR*(1-k0)*(1-span) + (max-phase_i)*ARR*gain。
     * k0=0.5 对称注入；0/1 保持原有零矢量分配行为。 */
    f_common = (f_arr * (1.0f - p_in->k0) * (1.0f - f_span)) +
        (f_max * f_scale);
    p_out->ccr_u = svpwm_tcm_to_ccr(f_common - (f_b * f_scale), p_in->arr);
    p_out->ccr_v = svpwm_tcm_to_ccr(f_common, p_in->arr);
    p_out->ccr_w = svpwm_tcm_to_ccr(f_common + (f_a * f_scale), p_in->arr);

    return SVPWM_OK;
}
