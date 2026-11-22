/******************************************************************************
 * Copyright (C) 2026 YA. All Rights Reserved.
 * @file current_loop.c
 * @author YA
 * @brief 电流环专用双轴 PI 和电压矢量限幅。
 * @note 四空格缩进，中文注释，UTF-8 编码。
 *****************************************************************************/
//****************************** Includes begin ******************************//
#include "../inc/current_loop.h"
#include <math.h>
#include <float.h>
#include <stddef.h>
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
/* 两轴共用积分方式，1 为梯形积分，0 为矩形积分。 */
#ifndef CURRENT_LOOP_TRAPEZOID_I
#define CURRENT_LOOP_TRAPEZOID_I 1U
#endif
#if (CURRENT_LOOP_TRAPEZOID_I != 0U) && (CURRENT_LOOP_TRAPEZOID_I != 1U)
#error CURRENT_LOOP_TRAPEZOID_I_must_be_0_or_1
#endif

#if defined(__CC_ARM)
#define CURRENT_LOOP_INLINE __forceinline
#elif defined(__GNUC__)
#define CURRENT_LOOP_INLINE inline __attribute__((always_inline))
#else
#define CURRENT_LOOP_INLINE inline
#endif
//******************************* Defines end ********************************//

//****************************** Structs begin *******************************//
/* 仅暂存本次计算结果，不复制配置和控制器历史。 */
typedef struct
{
    float f_integral;
    float f_i_term;
    float f_out;
} st_current_pi_result_t;
//******************************* Structs end *********************************//

//************************** Inline Functions begin **************************//
/** @brief 严格同号判断，零不参与同向积分回退。 */
static CURRENT_LOOP_INLINE bool current_pi_same_sign(float f_a, float f_b)
{
    return ((f_a > 0.0f) && (f_b > 0.0f)) ||
        ((f_a < 0.0f) && (f_b < 0.0f));
}

/** @brief 对称限幅，调用者保证上限有效。 */
static CURRENT_LOOP_INLINE float current_pi_clamp(float f_v, float f_max)
{
    if (f_v > f_max) { return f_max; }
    if (f_v < -f_max) { return -f_max; }
    return f_v;
}

/** @brief 计算单轴候选结果，不修改运行状态。 */
static CURRENT_LOOP_INLINE current_loop_status_t current_pi_calc(
    const st_current_pi_t *p_pi, float f_err, bool b_has_sample,
    st_current_pi_result_t *p_result)
{
    float f_p = p_pi->f_kp * f_err;
    float f_i_term;
    float f_i;
    float f_out;

#if CURRENT_LOOP_TRAPEZOID_I
    float f_prev_err = b_has_sample ? p_pi->f_err_1 : f_err;
    f_i_term = p_pi->f_i_gain * (f_err + f_prev_err);
#else
    (void)b_has_sample;
    f_i_term = p_pi->f_i_gain * f_err;
#endif
    f_i = p_pi->f_integral + f_i_term;
    /* 在限幅掩盖溢出前拒绝异常中间结果。 */
    if (!(f_p > -FLT_MAX && f_p < FLT_MAX) ||
        !(f_i_term > -FLT_MAX && f_i_term < FLT_MAX) ||
        !(f_i > -FLT_MAX && f_i < FLT_MAX))
    {
        return CURRENT_LOOP_ERR_STATE;
    }
    if (p_pi->f_int_max > 0.0f)
    {
        f_i = current_pi_clamp(f_i, p_pi->f_int_max);
    }
    f_out = f_p + f_i;
    if (!(f_out > -FLT_MAX && f_out < FLT_MAX))
    {
        return CURRENT_LOOP_ERR_STATE;
    }
    if (p_pi->f_out_max > 0.0f)
    {
        if ((fabsf(f_out) > p_pi->f_out_max) &&
            current_pi_same_sign(f_i_term, f_out))
        {
            f_i = p_pi->f_integral;
            f_i_term = 0.0f;
            f_out = f_p + f_i;
        }
        f_out = current_pi_clamp(f_out, p_pi->f_out_max);
    }
    p_result->f_integral = f_i;
    p_result->f_i_term = f_i_term;
    p_result->f_out = f_out;
    return CURRENT_LOOP_OK;
}
//*************************** Inline Functions end ***************************//

//************************** Static Functions begin **************************//
/** @brief 校验单轴配置并预计算积分系数，仅初始化时调用。 */
static current_loop_status_t current_pi_init(st_current_pi_t *p_pi,
    const st_current_pi_cfg_t *p_cfg)
{
    float f_i_gain;

    //-------------------------参数检查 begin-------------------------//
    if (!(p_cfg->f_kp >= 0.0f && p_cfg->f_kp < FLT_MAX) ||
        !(p_cfg->f_ki >= 0.0f && p_cfg->f_ki < FLT_MAX) ||
        !(p_cfg->f_dt > 0.0f && p_cfg->f_dt < FLT_MAX) ||
        !(p_cfg->f_out_max >= 0.0f && p_cfg->f_out_max < FLT_MAX) ||
        !(p_cfg->f_int_max >= 0.0f && p_cfg->f_int_max < FLT_MAX))
    {
        return CURRENT_LOOP_ERR_PARAM;
    }
    //--------------------------参数检查 end--------------------------//
#if CURRENT_LOOP_TRAPEZOID_I
    f_i_gain = p_cfg->f_ki * 0.5f * p_cfg->f_dt;
#else
    f_i_gain = p_cfg->f_ki * p_cfg->f_dt;
#endif
    if (!(f_i_gain >= 0.0f && f_i_gain < FLT_MAX))
    {
        return CURRENT_LOOP_ERR_PARAM;
    }
    p_pi->f_kp = p_cfg->f_kp;
    p_pi->f_i_gain = f_i_gain;
    p_pi->f_out_max = p_cfg->f_out_max;
    p_pi->f_int_max = p_cfg->f_int_max;
    return CURRENT_LOOP_OK;
}
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
/** @brief 校验两轴配置，初始化状态及电压限幅平方，失败不修改对象。 */
current_loop_status_t current_loop_init(st_current_loop_t *p_loop,
    const st_current_pi_cfg_t *p_d_cfg,
    const st_current_pi_cfg_t *p_q_cfg, float f_voltage_limit_v)
{
    st_current_loop_t st_init = {0};

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_loop) || (NULL == p_d_cfg) || (NULL == p_q_cfg) ||
        !(f_voltage_limit_v > 0.0f && f_voltage_limit_v < FLT_MAX))
    {
        return CURRENT_LOOP_ERR_PARAM;
    }
    //--------------------------参数检查 end--------------------------//
    st_init.f_voltage_limit_sq = f_voltage_limit_v * f_voltage_limit_v;
    if (!(st_init.f_voltage_limit_sq > 0.0f &&
          st_init.f_voltage_limit_sq < FLT_MAX) ||
        (CURRENT_LOOP_OK != current_pi_init(&st_init.st_d_pi, p_d_cfg)) ||
        (CURRENT_LOOP_OK != current_pi_init(&st_init.st_q_pi, p_q_cfg)))
    {
        return CURRENT_LOOP_ERR_PARAM;
    }
    st_init.f_voltage_limit_v = f_voltage_limit_v;
    st_init.b_init = true;
    *p_loop = st_init;
    return CURRENT_LOOP_OK;
}

/** @brief 清空历史，保留配置和预计算系数。 */
current_loop_status_t current_loop_reset(st_current_loop_t *p_loop)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_loop) { return CURRENT_LOOP_ERR_PARAM; }
    if (!p_loop->b_init) { return CURRENT_LOOP_ERR_STATE; }
    //--------------------------参数检查 end--------------------------//
    p_loop->st_d_pi.f_integral = 0.0f;
    p_loop->st_d_pi.f_err_1 = 0.0f;
    p_loop->st_q_pi.f_integral = 0.0f;
    p_loop->st_q_pi.f_err_1 = 0.0f;
    p_loop->b_has_sample = false;
    return CURRENT_LOOP_OK;
}

/** @brief 计算两轴 PI、矢量限幅和积分反馈，全部成功后提交状态。 */
current_loop_status_t current_loop_calc(st_current_loop_t *p_loop,
    float f_id_ref, float f_iq_ref, float f_id, float f_iq,
    float *p_vd, float *p_vq)
{
    st_current_pi_result_t st_d;
    st_current_pi_result_t st_q;
    float f_ed = f_id_ref - f_id;
    float f_eq = f_iq_ref - f_iq;
    float f_norm_sq;
    float f_scale;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_loop) || (NULL == p_vd) || (NULL == p_vq))
    {
        return CURRENT_LOOP_ERR_PARAM;
    }
    if (!p_loop->b_init) { return CURRENT_LOOP_ERR_STATE; }
    /* 非有限输入的差值也非有限，同时检查减法溢出。 */
    if (!(f_ed > -FLT_MAX && f_ed < FLT_MAX) ||
        !(f_eq > -FLT_MAX && f_eq < FLT_MAX))
    {
        return CURRENT_LOOP_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//
    if ((CURRENT_LOOP_OK != current_pi_calc(&p_loop->st_d_pi,
            f_ed, p_loop->b_has_sample, &st_d)) ||
        (CURRENT_LOOP_OK != current_pi_calc(&p_loop->st_q_pi,
            f_eq, p_loop->b_has_sample, &st_q)))
    {
        return CURRENT_LOOP_ERR_STATE;
    }
    f_norm_sq = st_d.f_out * st_d.f_out + st_q.f_out * st_q.f_out;
    if (!(f_norm_sq >= 0.0f && f_norm_sq < FLT_MAX))
    {
        return CURRENT_LOOP_ERR_STATE;
    }
    if (f_norm_sq > p_loop->f_voltage_limit_sq)
    {
        f_scale = p_loop->f_voltage_limit_v / sqrtf(f_norm_sq);
        /* 保留原先严格小于 1 时才回退积分的规则。 */
        if (f_scale < 1.0f)
        {
            if (current_pi_same_sign(f_ed, st_d.f_out) &&
                current_pi_same_sign(st_d.f_i_term, st_d.f_out))
            {
                st_d.f_integral = p_loop->st_d_pi.f_integral;
            }
            if (current_pi_same_sign(f_eq, st_q.f_out) &&
                current_pi_same_sign(st_q.f_i_term, st_q.f_out))
            {
                st_q.f_integral = p_loop->st_q_pi.f_integral;
            }
        }
        st_d.f_out *= f_scale;
        st_q.f_out *= f_scale;
    }
    p_loop->st_d_pi.f_integral = st_d.f_integral;
    p_loop->st_d_pi.f_err_1 = f_ed;
    p_loop->st_q_pi.f_integral = st_q.f_integral;
    p_loop->st_q_pi.f_err_1 = f_eq;
    p_loop->b_has_sample = true;
    *p_vd = st_d.f_out;
    *p_vq = st_q.f_out;
    return CURRENT_LOOP_OK;
}
//************************* Function Definitions end *************************//
