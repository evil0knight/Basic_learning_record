/******************************************************************************
 * Copyright (C) 2026 YA. All Rights Reserved.
 * @file position_loop.c
 * @par dependencies position_loop.h, math.h, stddef.h
 * @author YA
 * @brief 单圈最短路径位置环及反馈微分滤波。
 * Processing flow: init -> set_position_ref -> calc -> reset。
 * @version V2.0
 * @note 四空格缩进，中文注释，UTF-8。
 *****************************************************************************/
//****************************** Includes begin ******************************//
#include "../inc/position_loop.h"
#include <math.h>
#include <stddef.h>
//******************************* Includes end *******************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 校验配置并预计算积分、微分及滤波系数。
 * Steps: 校验配置 -> 预计算固定系数 -> 提交初始化对象。
 * @param[out] p_loop : 调用者持有的运行对象。
 * @param[in] p_cfg : 增益、周期、限幅和微分滤波配置。
 * @return position_loop_status_t : 失败时保留原对象。
 */
position_loop_status_t position_loop_init(st_position_loop_t *p_loop,
    const st_position_loop_cfg_t *p_cfg)
{
    st_position_loop_t st_init = {0};
    float f_sum;
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_loop) || (NULL == p_cfg) ||
        !isfinite(p_cfg->f_kp) || !(p_cfg->f_kp >= 0.0f) ||
        !isfinite(p_cfg->f_ki) || !(p_cfg->f_ki >= 0.0f) ||
        !isfinite(p_cfg->f_kd) || !(p_cfg->f_kd >= 0.0f) ||
        !((p_cfg->f_kp > 0.0f) || (p_cfg->f_ki > 0.0f) ||
          (p_cfg->f_kd > 0.0f)) ||
        !isfinite(p_cfg->f_dt) || !(p_cfg->f_dt > 0.0f) ||
        !isfinite(p_cfg->f_out_max) || !(p_cfg->f_out_max > 0.0f) ||
        !isfinite(p_cfg->f_int_max) || !(p_cfg->f_int_max >= 0.0f) ||
        !isfinite(p_cfg->f_d_rc) || !(p_cfg->f_d_rc >= 0.0f))
    {
        return POSITION_LOOP_ERR_PARAM;
    }
    //--------------------------参数检查 end--------------------------//
    f_sum = p_cfg->f_dt + p_cfg->f_d_rc;
    st_init.f_i_gain = p_cfg->f_ki * p_cfg->f_dt;
    st_init.f_d_gain = p_cfg->f_kd / p_cfg->f_dt;
    st_init.f_d_alpha = p_cfg->f_dt / f_sum;
    if (!isfinite(f_sum) || !isfinite(st_init.f_i_gain) ||
        !isfinite(st_init.f_d_gain) ||
        !(st_init.f_d_alpha > 0.0f))
    {
        return POSITION_LOOP_ERR_PARAM;
    }
    st_init.f_kp = p_cfg->f_kp;
    st_init.f_out_max = p_cfg->f_out_max;
    st_init.f_int_max = p_cfg->f_int_max;
    st_init.b_init = true;
    *p_loop = st_init;
    return POSITION_LOOP_OK;
}

/**
 * @brief 清除积分和微分历史，保留位置目标及配置。
 * @param[in,out] p_loop : 已初始化的运行对象。
 * @return position_loop_status_t : 参数或初始化状态检查结果。
 */
position_loop_status_t position_loop_reset(st_position_loop_t *p_loop)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_loop) { return POSITION_LOOP_ERR_PARAM; }
    if (!p_loop->b_init) { return POSITION_LOOP_ERR_STATE; }
    //--------------------------参数检查 end--------------------------//
    p_loop->f_integral = 0.0f;
    p_loop->f_d_prev = 0.0f;
    p_loop->f_feedback_prev = 0.0f;
    p_loop->b_has_sample = false;
    return POSITION_LOOP_OK;
}

/**
 * @brief 保存显式位置目标，不执行控制计算。
 * @param[in,out] p_loop : 已初始化的运行对象。
 * @param[in] f_position_ref_rad : [0, 2π) 机械位置目标/rad。
 * @return position_loop_status_t : 非法目标不修改对象。
 * @note 与控制中断共享对象时，调用方用短临界区保护。
 */
position_loop_status_t position_loop_set_position_ref(
    st_position_loop_t *p_loop, float f_position_ref_rad)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_loop) || !isfinite(f_position_ref_rad) ||
        !(f_position_ref_rad >= 0.0f) ||
        !(f_position_ref_rad < POSITION_LOOP_TWO_PI))
    {
        return POSITION_LOOP_ERR_PARAM;
    }
    if (!p_loop->b_init) { return POSITION_LOOP_ERR_STATE; }
    //--------------------------参数检查 end--------------------------//
    p_loop->f_position_ref_rad = f_position_ref_rad;
    p_loop->b_target_valid = true;
    return POSITION_LOOP_OK;
}

/**
 * @brief 使用保存的目标计算最短路径位置控制，反馈微分处理回绕。
 * Steps: 最短路径误差 -> PI 和反馈微分 -> 抗饱和 -> 限幅提交。
 * @param[in,out] p_loop : 已初始化且已保存目标的运行对象。
 * @param[in] f_feedback_rad : [0, 2π) 机械位置反馈/rad。
 * @param[out] p_speed_ref : 机械速度目标/rad/s，地址独立于状态。
 * @return position_loop_status_t : 失败时不修改状态及输出。
 */
position_loop_status_t position_loop_calc(st_position_loop_t *p_loop,
    float f_feedback_rad, float *p_speed_ref)
{
    float f_err;
    float f_delta;
    float f_p;
    float f_i;
    float f_d = 0.0f;
    float f_step = 0.0f;
    float f_out;
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_loop) || (NULL == p_speed_ref) ||
        !isfinite(f_feedback_rad) || !(f_feedback_rad >= 0.0f) ||
        !(f_feedback_rad < POSITION_LOOP_TWO_PI))
    {
        return POSITION_LOOP_ERR_PARAM;
    }
    if (!p_loop->b_init) { return POSITION_LOOP_ERR_STATE; }
    //--------------------------参数检查 end--------------------------//
    /* 调试器可直接改写目标，计算入口同样拒绝单圈范围外的值。 */
    if (!isfinite(p_loop->f_position_ref_rad) ||
        !(p_loop->f_position_ref_rad >= 0.0f) ||
        !(p_loop->f_position_ref_rad < POSITION_LOOP_TWO_PI))
    {
        return POSITION_LOOP_ERR_PARAM;
    }
    f_err = p_loop->f_position_ref_rad - f_feedback_rad;
    if (f_err >= POSITION_LOOP_PI) { f_err -= POSITION_LOOP_TWO_PI; }
    if (f_err < -POSITION_LOOP_PI) { f_err += POSITION_LOOP_TWO_PI; }
    f_p = p_loop->f_kp * f_err;
    f_i = p_loop->f_integral;
    if (0.0f != p_loop->f_i_gain)
    {
        f_step = p_loop->f_i_gain * f_err;
        f_i += f_step;
    }
    if ((0.0f != p_loop->f_d_gain) && p_loop->b_has_sample)
    {
        f_delta = f_feedback_rad - p_loop->f_feedback_prev;
        if (f_delta >= POSITION_LOOP_PI)
        {
            f_delta -= POSITION_LOOP_TWO_PI;
        }
        if (f_delta < -POSITION_LOOP_PI)
        {
            f_delta += POSITION_LOOP_TWO_PI;
        }
        f_d = p_loop->f_d_prev + p_loop->f_d_alpha *
            (-p_loop->f_d_gain * f_delta - p_loop->f_d_prev);
    }
    if (!isfinite(f_p) || !isfinite(f_step) ||
        !isfinite(f_i) || !isfinite(f_d))
    {
        return POSITION_LOOP_ERR_STATE;
    }
    if (p_loop->f_int_max > 0.0f)
    {
        if (f_i > p_loop->f_int_max) { f_i = p_loop->f_int_max; }
        if (f_i < -p_loop->f_int_max) { f_i = -p_loop->f_int_max; }
    }
    f_out = f_p + f_i + f_d;
    if (!isfinite(f_out)) { return POSITION_LOOP_ERR_STATE; }
    if (((f_out > p_loop->f_out_max) && (f_step > 0.0f)) ||
        ((f_out < -p_loop->f_out_max) && (f_step < 0.0f)))
    {
        f_i = p_loop->f_integral;
        f_out = f_p + f_i + f_d;
    }
    if (!isfinite(f_out)) { return POSITION_LOOP_ERR_STATE; }
    if (f_out > p_loop->f_out_max) { f_out = p_loop->f_out_max; }
    if (f_out < -p_loop->f_out_max) { f_out = -p_loop->f_out_max; }
    p_loop->f_integral = f_i;
    p_loop->f_d_prev = f_d;
    p_loop->f_feedback_prev = f_feedback_rad;
    p_loop->b_has_sample = true;
    *p_speed_ref = f_out;
    return POSITION_LOOP_OK;
}
//************************* Function Definitions end *************************//
