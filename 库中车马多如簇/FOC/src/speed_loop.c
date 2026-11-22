/******************************************************************************
 * Copyright (C) 2026 YA. All Rights Reserved.
 * @file speed_loop.c
 * @par dependencies speed_loop.h, math.h, stddef.h
 * @author YA
 * @brief 专用速度 PI，预计算积分系数及抗饱和。
 * Processing flow: init -> calc -> reset。
 * @version V2.0
 * @note 四空格缩进，中文注释，UTF-8。
 *****************************************************************************/
//****************************** Includes begin ******************************//
#include "../inc/speed_loop.h"
#include <math.h>
#include <stddef.h>
//******************************* Includes end *******************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 校验参数并预计算矩形积分系数。
 * Steps: 检查配置 -> 计算 Ki*dt -> 提交初始化对象。
 * @param[out] p_loop : 调用者持有的运行对象。
 * @param[in] p_cfg : 增益、周期和限幅配置。
 * @return speed_loop_status_t : 失败时保留原对象。
 */
speed_loop_status_t speed_loop_init(st_speed_loop_t *p_loop,
    const st_speed_loop_cfg_t *p_cfg)
{
    st_speed_loop_t st_init = {0};
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_loop) || (NULL == p_cfg) ||
        !isfinite(p_cfg->f_kp) || !(p_cfg->f_kp >= 0.0f) ||
        !isfinite(p_cfg->f_ki) || !(p_cfg->f_ki >= 0.0f) ||
        !((p_cfg->f_kp > 0.0f) || (p_cfg->f_ki > 0.0f)) ||
        !isfinite(p_cfg->f_dt) || !(p_cfg->f_dt > 0.0f) ||
        !isfinite(p_cfg->f_out_max) || !(p_cfg->f_out_max > 0.0f) ||
        !isfinite(p_cfg->f_int_max) || !(p_cfg->f_int_max >= 0.0f))
    {
        return SPEED_LOOP_ERR_PARAM;
    }
    //--------------------------参数检查 end--------------------------//
    st_init.f_i_gain = p_cfg->f_ki * p_cfg->f_dt;
    if (!isfinite(st_init.f_i_gain)) { return SPEED_LOOP_ERR_PARAM; }
    st_init.f_kp = p_cfg->f_kp;
    st_init.f_out_max = p_cfg->f_out_max;
    st_init.f_int_max = p_cfg->f_int_max;
    st_init.b_init = true;
    *p_loop = st_init;
    return SPEED_LOOP_OK;
}

/**
 * @brief 清除积分历史，保留配置。
 * @param[in,out] p_loop : 已初始化的运行对象。
 * @return speed_loop_status_t : 参数或初始化状态检查结果。
 */
speed_loop_status_t speed_loop_reset(st_speed_loop_t *p_loop)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_loop) { return SPEED_LOOP_ERR_PARAM; }
    if (!p_loop->b_init) { return SPEED_LOOP_ERR_STATE; }
    //--------------------------参数检查 end--------------------------//
    p_loop->f_integral = 0.0f;
    return SPEED_LOOP_OK;
}

/**
 * @brief 将机械速度误差转换为 Iq，限幅时回退同向积分。
 * Steps: 检查输入 -> 计算候选 PI -> 抗饱和及限幅 -> 提交。
 * @param[in,out] p_loop : 已初始化的运行对象。
 * @param[in] f_ref : 机械速度目标/rad/s。
 * @param[in] f_feedback : 同方向机械速度反馈/rad/s。
 * @param[out] p_iq : Iq 输出/A，地址须独立于运行状态。
 * @return speed_loop_status_t : 失败时不修改状态及输出。
 */
speed_loop_status_t speed_loop_calc(st_speed_loop_t *p_loop,
    float f_ref, float f_feedback, float *p_iq)
{
    float f_err;
    float f_p;
    float f_i;
    float f_step = 0.0f;
    float f_out;
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_loop) || (NULL == p_iq))
    {
        return SPEED_LOOP_ERR_PARAM;
    }
    if (!p_loop->b_init) { return SPEED_LOOP_ERR_STATE; }
    if (!isfinite(f_ref) || !isfinite(f_feedback))
    {
        return SPEED_LOOP_ERR_PARAM;
    }
    //--------------------------参数检查 end--------------------------//
    f_err = f_ref - f_feedback;
    f_p = p_loop->f_kp * f_err;
    f_i = p_loop->f_integral;
    if (0.0f != p_loop->f_i_gain)
    {
        f_step = p_loop->f_i_gain * f_err;
        f_i += f_step;
    }
    if (!isfinite(f_err) || !isfinite(f_p) ||
        !isfinite(f_step) || !isfinite(f_i))
    {
        return SPEED_LOOP_ERR_STATE;
    }
    if (p_loop->f_int_max > 0.0f)
    {
        if (f_i > p_loop->f_int_max) { f_i = p_loop->f_int_max; }
        if (f_i < -p_loop->f_int_max) { f_i = -p_loop->f_int_max; }
    }
    f_out = f_p + f_i;
    if (!isfinite(f_out)) { return SPEED_LOOP_ERR_STATE; }
    if (((f_out > p_loop->f_out_max) && (f_step > 0.0f)) ||
        ((f_out < -p_loop->f_out_max) && (f_step < 0.0f)))
    {
        f_i = p_loop->f_integral;
        f_out = f_p + f_i;
    }
    if (!isfinite(f_out)) { return SPEED_LOOP_ERR_STATE; }
    if (f_out > p_loop->f_out_max) { f_out = p_loop->f_out_max; }
    if (f_out < -p_loop->f_out_max) { f_out = -p_loop->f_out_max; }
    p_loop->f_integral = f_i;
    *p_iq = f_out;
    return SPEED_LOOP_OK;
}
//************************* Function Definitions end *************************//
