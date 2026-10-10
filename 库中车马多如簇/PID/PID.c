/******************************************************************************
 * Copyright (C) 2026 Fangzhou Technology, Inc. or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file PID.c
 *
 * @par dependencies
 * - PID.h
 * - math.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 位置式/增量式单环 PID 实现。
 *
 * Processing flow:
 *
 *     calc -> (位置式|增量式) -> 滤波/限幅 -> 堵转判定 -> 提交历史
 *
 * @version V3.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes*********************************//

#include "PID.h"

#include <math.h>
#include <stddef.h>

//********************************Includes*********************************//

/* ----------------------------- 静态辅助 ----------------------------- */

/**
 * @brief 校验配置合法性。
 *
 * Steps:
 *  1. 检查空指针与所有浮点数的有限性。
 *  2. 检查非负约束及比例范围。
 *
 * @param[in] p_cfg : 待校验的配置。
 *
 * @return pid_status_t : 成功或 PID_ERR_PARAM。
 */
static pid_status_t validate_cfg(const pid_cfg_t * const p_cfg)
{
    if (NULL == p_cfg) {
        return PID_ERR_NULL;
    }
    if ((!isfinite(p_cfg->f_kp)) || (!isfinite(p_cfg->f_ki))
        || (!isfinite(p_cfg->f_kd)) || (!isfinite(p_cfg->f_dt))
        || (!isfinite(p_cfg->f_out_max)) || (!isfinite(p_cfg->f_int_max))
        || (!isfinite(p_cfg->f_dead_band))
        || (!isfinite(p_cfg->f_i_sep_err))
        || (!isfinite(p_cfg->f_vsi_a)) || (!isfinite(p_cfg->f_vsi_b))
        || (!isfinite(p_cfg->f_d_rc)) || (!isfinite(p_cfg->f_out_rc))
        || (!isfinite(p_cfg->f_stall_out_ratio))
        || (!isfinite(p_cfg->f_stall_target_min))
        || (!isfinite(p_cfg->f_stall_err_ratio))) {
        return PID_ERR_PARAM;
    }
    if ((p_cfg->f_kp < 0.0f) || (p_cfg->f_ki < 0.0f)
        || (p_cfg->f_kd < 0.0f) || (p_cfg->f_dt <= 0.0f)
        || (p_cfg->f_out_max < 0.0f) || (p_cfg->f_int_max < 0.0f)
        || (p_cfg->f_dead_band < 0.0f) || (p_cfg->f_i_sep_err < 0.0f)
        || (p_cfg->f_vsi_a < 0.0f) || (p_cfg->f_vsi_b < 0.0f)
        || (p_cfg->f_d_rc < 0.0f) || (p_cfg->f_out_rc < 0.0f)
        || (p_cfg->f_stall_out_ratio < 0.0f)
        || (p_cfg->f_stall_out_ratio > 1.0f)
        || (p_cfg->f_stall_target_min < 0.0f)
        || (p_cfg->f_stall_err_ratio < 0.0f)) {
        return PID_ERR_PARAM;
    }
    if ((PID_POSITION != p_cfg->e_type)
        && (PID_INCREMENT != p_cfg->e_type)) {
        return PID_ERR_PARAM;
    }
    return PID_OK;
}

/**
 * @brief 对称限幅至 [-f_m, +f_m]。
 */
static float clamp_sym(const float f_v, const float f_m)
{
    if (f_v > f_m) {
        return f_m;
    }
    if (f_v < -f_m) {
        return -f_m;
    }
    return f_v;
}

/**
 * @brief 一阶低通,alpha = dt / (rc + dt)。
 */
static float lpf1(const float f_in, const float f_prev,
                  const float f_rc, const float f_dt)
{
    const float f_alpha = f_dt / (f_rc + f_dt);
    return (f_alpha * f_in) + ((1.0f - f_alpha) * f_prev);
}

/**
 * @brief 两值同号(严格大于/小于 0)。
 */
static bool same_sign(const float f_a, const float f_b)
{
    return ((f_a > 0.0f) && (f_b > 0.0f))
        || ((f_a < 0.0f) && (f_b < 0.0f));
}

/**
 * @brief 计算本次原始积分增量(含分离、梯形)。
 */
static float i_term_raw(const pid_cfg_t * const p_c,
                        const float f_e, const float f_err_1)
{
    if ((0.0f < p_c->f_i_sep_err)
        && (fabsf(f_e) > p_c->f_i_sep_err)) {
        return 0.0f;
    }
    if (p_c->b_trapezoid_i) {
        return p_c->f_ki * 0.5f * (f_e + f_err_1) * p_c->f_dt;
    }
    return p_c->f_ki * f_e * p_c->f_dt;
}

/**
 * @brief 变速积分:同向且误差超阈值时按过渡宽度衰减增量。
 */
static float vsi_scale(const pid_cfg_t * const p_c, const float f_e,
                       const float f_i_term, const float f_ref)
{
    float f_ae;

    if (p_c->f_vsi_a <= 0.0f) {
        return f_i_term;
    }
    if (!same_sign(f_e, f_ref)) {
        return f_i_term;
    }
    f_ae = fabsf(f_e);
    if (f_ae <= p_c->f_vsi_b) {
        return f_i_term;
    }
    if (f_ae < (p_c->f_vsi_a + p_c->f_vsi_b)) {
        return f_i_term
            * (1.0f - ((f_ae - p_c->f_vsi_b) / p_c->f_vsi_a));
    }
    return 0.0f;
}

/**
 * @brief 计算原始微分(未滤波)。
 */
static float d_raw(const pid_cfg_t * const p_c, const float f_e,
                   const float f_err_1,
                   const float f_m, const float f_m_1)
{
    if (0.0f == p_c->f_kd) {
        return 0.0f;
    }
    if (p_c->b_d_on_measure) {
        return -p_c->f_kd * (f_m - f_m_1) / p_c->f_dt;
    }
    return p_c->f_kd * (f_e - f_err_1) / p_c->f_dt;
}

/**
 * @brief 位置式单周期计算。
 *
 * Steps:
 *  1. 死区分支:积分保留、输出零、增量清零。
 *  2. 计算 P/I/D,应用变速积分、积分限幅、微分滤波。
 *  3. 条件积分抗饱和,输出滤波与限幅。
 */
static pid_status_t calc_position(pid_t * const p_pid,
                                  const float f_e, const float f_measure)
{
    const pid_cfg_t * const p_c = &p_pid->s_cfg;
    float f_p;
    float f_i;
    float f_i_prev;
    float f_d;
    float f_u;

    if ((0.0f < p_c->f_dead_band)
        && (fabsf(f_e) <= p_c->f_dead_band)) {
        p_pid->f_integral_term = 0.0f;
        p_pid->f_integral_prev = p_pid->f_integral;
        p_pid->f_out = 0.0f;
        return PID_OK;
    }

    if (p_c->b_p_on_measure) {
        p_pid->f_proportional -= p_c->f_kp
            * (f_measure - p_pid->f_measure_1);
        f_p = p_pid->f_proportional;
    } else {
        f_p = p_c->f_kp * f_e;
    }

    p_pid->f_integral_term = i_term_raw(p_c, f_e, p_pid->f_err_1);
    p_pid->f_integral_term = vsi_scale(p_c, f_e,
        p_pid->f_integral_term, p_pid->f_integral);
    f_i_prev = p_pid->f_integral;
    f_i = p_pid->f_integral + p_pid->f_integral_term;
    if (0.0f < p_c->f_int_max) {
        f_i = clamp_sym(f_i, p_c->f_int_max);
    }

    f_d = d_raw(p_c, f_e, p_pid->f_err_1, f_measure, p_pid->f_measure_1);
    if (0.0f < p_c->f_d_rc) {
        f_d = lpf1(f_d, p_pid->f_d_1, p_c->f_d_rc, p_c->f_dt);
    }

    f_u = f_p + f_i + f_d;

    if ((0.0f < p_c->f_out_max) && (fabsf(f_u) > p_c->f_out_max)
        && same_sign(p_pid->f_integral_term, f_u)) {
        f_i = f_i_prev;
        p_pid->f_integral_term = 0.0f;
        f_u = f_p + f_i + f_d;
    }

    if (0.0f < p_c->f_out_rc) {
        f_u = lpf1(f_u, p_pid->f_out_1, p_c->f_out_rc, p_c->f_dt);
    }
    if (0.0f < p_c->f_out_max) {
        f_u = clamp_sym(f_u, p_c->f_out_max);
    }

    if ((!isfinite(f_u)) || (!isfinite(f_i))) {
        return PID_ERR_PARAM;
    }
    p_pid->f_integral_prev = f_i_prev;
    p_pid->f_integral = f_i;
    p_pid->f_d_1 = f_d;
    p_pid->f_out = f_u;
    return PID_OK;
}

/**
 * @brief 增量式单周期计算。
 *
 * Steps:
 *  1. 死区分支:保持当前输出。
 *  2. 分别计算 ΔP、单周期积分分量、Δ微分并累加。
 *  3. 输出滤波与限幅(限幅即抗饱和)。
 */
static pid_status_t calc_increment(pid_t * const p_pid,
                                   const float f_e, const float f_measure)
{
    const pid_cfg_t * const p_c = &p_pid->s_cfg;
    float f_dp;
    float f_i_term;
    float f_d_curr;
    float f_dd;
    float f_u;

    if ((0.0f < p_c->f_dead_band)
        && (fabsf(f_e) <= p_c->f_dead_band)) {
        p_pid->f_integral_term = 0.0f;
        return PID_OK;
    }

    if (p_c->b_p_on_measure) {
        f_dp = -p_c->f_kp * (f_measure - p_pid->f_measure_1);
    } else {
        f_dp = p_c->f_kp * (f_e - p_pid->f_err_1);
    }

    f_i_term = i_term_raw(p_c, f_e, p_pid->f_err_1);
    f_i_term = vsi_scale(p_c, f_e, f_i_term, f_e);
    p_pid->f_integral_term = f_i_term;

    f_d_curr = d_raw(p_c, f_e, p_pid->f_err_1,
        f_measure, p_pid->f_measure_1);
    if (0.0f < p_c->f_d_rc) {
        f_d_curr = lpf1(f_d_curr, p_pid->f_d_1,
            p_c->f_d_rc, p_c->f_dt);
    }
    f_dd = f_d_curr - p_pid->f_d_1;

    f_u = p_pid->f_out + f_dp + f_i_term + f_dd;

    if (0.0f < p_c->f_out_rc) {
        f_u = lpf1(f_u, p_pid->f_out_1, p_c->f_out_rc, p_c->f_dt);
    }
    if (0.0f < p_c->f_out_max) {
        f_u = clamp_sym(f_u, p_c->f_out_max);
    }

    if (!isfinite(f_u)) {
        return PID_ERR_PARAM;
    }
    p_pid->f_d_1 = f_d_curr;
    p_pid->f_out = f_u;
    return PID_OK;
}

/**
 * @brief 堵转判定:输出高、目标够大、相对误差超阈值,持续累计。
 */
static void stall_update(pid_t * const p_pid,
                         const float f_e, const float f_target)
{
    const pid_cfg_t * const p_c = &p_pid->s_cfg;
    float f_abs_t;
    bool  b_out_high;
    bool  b_target_valid;
    bool  b_err_bad;

    if (0U == p_c->u32_stall_count) {
        p_pid->u32_stall_cnt = 0U;
        p_pid->b_stalled = false;
        return;
    }

    f_abs_t = fabsf(f_target);
    b_out_high = (0.0f < p_c->f_out_max)
        && (fabsf(p_pid->f_out)
            >= (p_c->f_out_max * p_c->f_stall_out_ratio));
    b_target_valid = (f_abs_t >= p_c->f_stall_target_min)
        && (p_c->f_stall_target_min > 0.0f);
    b_err_bad = (f_abs_t > 0.0f)
        && ((fabsf(f_e) / f_abs_t) > p_c->f_stall_err_ratio);

    if (b_out_high && b_target_valid && b_err_bad) {
        if (p_pid->u32_stall_cnt < p_c->u32_stall_count) {
            p_pid->u32_stall_cnt++;
        }
        if (p_pid->u32_stall_cnt >= p_c->u32_stall_count) {
            p_pid->b_stalled = true;
        }
    } else {
        p_pid->u32_stall_cnt = 0U;
        p_pid->b_stalled = false;
    }
}

/* ----------------------------- 公共接口 ----------------------------- */

/**
 * @brief 校验配置并初始化状态。
 *
 * Steps:
 *  1. 空指针与配置合法性检查。
 *  2. 清空运行时并保存配置副本。
 *
 * @param[out] p_pid : 控制器状态。
 * @param[in]  p_cfg : 配置。
 *
 * @return pid_status_t : 成功或错误码。
 */
pid_status_t pid_init(pid_t * const p_pid, const pid_cfg_t * const p_cfg)
{
    pid_status_t e_st;

    if ((NULL == p_pid) || (NULL == p_cfg)) {
        return PID_ERR_NULL;
    }
    e_st = validate_cfg(p_cfg);
    if (PID_OK != e_st) {
        return e_st;
    }
    *p_pid = (pid_t){0};
    p_pid->s_cfg = *p_cfg;
    p_pid->b_init = true;
    return PID_OK;
}

/**
 * @brief 复位历史,保留配置。
 */
pid_status_t pid_reset(pid_t * const p_pid)
{
    pid_cfg_t s_bak;

    if (NULL == p_pid) {
        return PID_ERR_NULL;
    }
    if (!p_pid->b_init) {
        return PID_ERR_STATE;
    }
    s_bak = p_pid->s_cfg;
    *p_pid = (pid_t){0};
    p_pid->s_cfg = s_bak;
    p_pid->b_init = true;
    return PID_OK;
}

/**
 * @brief 本周期控制量计算,临时副本执行,失败不污染状态。
 *
 * Steps:
 *  1. 校验指针、初始化、数值有限性。
 *  2. 首次采样用本次填充历史;分发位置式/增量式。
 *  3. 堵转判定并提交历史。
 */
pid_status_t pid_calc(
    pid_t * const   p_pid,
    const float     f_target,
    const float     f_measure,
    float * const   p_out)
{
    pid_status_t e_st;
    pid_t        s_tmp;
    float        f_e;

    if ((NULL == p_pid) || (NULL == p_out)) {
        return PID_ERR_NULL;
    }
    if (!p_pid->b_init) {
        return PID_ERR_STATE;
    }
    if ((!isfinite(f_target)) || (!isfinite(f_measure))) {
        return PID_ERR_PARAM;
    }

    s_tmp = *p_pid;
    f_e = f_target - f_measure;
    if (!isfinite(f_e)) {
        return PID_ERR_PARAM;
    }

    if (!s_tmp.b_has_sample) {
        s_tmp.f_err_1 = f_e;
        s_tmp.f_err_2 = f_e;
        s_tmp.f_measure_1 = f_measure;
        s_tmp.f_measure_2 = f_measure;
        s_tmp.b_has_sample = true;
    }

    if (PID_POSITION == s_tmp.s_cfg.e_type) {
        e_st = calc_position(&s_tmp, f_e, f_measure);
    } else {
        e_st = calc_increment(&s_tmp, f_e, f_measure);
    }
    if (PID_OK != e_st) {
        return e_st;
    }

    stall_update(&s_tmp, f_e, f_target);

    s_tmp.f_err_2 = s_tmp.f_err_1;
    s_tmp.f_err_1 = f_e;
    s_tmp.f_measure_2 = s_tmp.f_measure_1;
    s_tmp.f_measure_1 = f_measure;
    s_tmp.f_out_1 = s_tmp.f_out;
    s_tmp.b_sat_pending = true;

    *p_pid = s_tmp;
    *p_out = s_tmp.f_out;
    return PID_OK;
}

/**
 * @brief 执行器饱和反馈;仅位置式回退本周期积分。
 *
 * Steps:
 *  1. 校验指针、初始化、缩放范围。
 *  2. 增量式或未饱和直接返回;同向饱和则恢复积分。
 */
pid_status_t pid_sat_feedback(pid_t * const p_pid, const float f_scale)
{
    if (NULL == p_pid) {
        return PID_ERR_NULL;
    }
    if (!p_pid->b_init) {
        return PID_ERR_STATE;
    }
    if ((!isfinite(f_scale)) || (f_scale < 0.0f) || (f_scale > 1.0f)) {
        return PID_ERR_PARAM;
    }
    if (!p_pid->b_sat_pending) {
        return PID_OK;
    }
    p_pid->b_sat_pending = false;

    if (PID_INCREMENT == p_pid->s_cfg.e_type) {
        return PID_OK;
    }
    if (f_scale >= 1.0f) {
        return PID_OK;
    }
    if (same_sign(p_pid->f_err_1, p_pid->f_out)
        && same_sign(p_pid->f_integral_term, p_pid->f_out)) {
        p_pid->f_integral = p_pid->f_integral_prev;
        p_pid->f_integral_term = 0.0f;
    }
    return PID_OK;
}
