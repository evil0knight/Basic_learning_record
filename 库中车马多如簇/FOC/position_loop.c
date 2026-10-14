/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file position_loop.c
 *
 * @par dependencies
 * - PID.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief FOC位置环模块: 单PI(可选微分先行)把位置误差转成速度指令.
 *
 * ============================================================================
 *                              调用方式
 * ============================================================================
 *
 *   // 1. 启动时(main里一次)
 *   position_loop_init();
 *
 *   // 2. 位置环定时任务里(1kHz, 比速度环再慢5~10倍), 拿到目标/反馈位置后调用
 *   float omega_ref;
 *   position_loop_calc(pos_ref, pos_fb, &omega_ref);
 *   // 把omega_ref送给速度环当参考
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

#include "PID.h"

//********************************Includes***********************************//


//*********************************Defines***********************************//

#define PL_TS          (0.001f)         /* 1kHz位置环, 电流环的1/10         */
#define PL_OMEGA_MAX   (300.0f)         /* 速度指令上限 (rad/s机械角速度)   */

//*********************************Defines***********************************//


//*******************************Enumerations********************************//

typedef enum
{
    POSITION_LOOP_OK       = 0,
    POSITION_LOOP_ERR_NULL = 1
} position_loop_status_t;

//*******************************Enumerations********************************//


//*****************************Static Variables******************************//

static pid_t g_s_pos_pid;

/* 位置环典型做P或PID,积分慎用(容易超调).
 * 微分先行(b_d_on_measure)避免阶跃指令时微分冲击. */
static const pid_cfg_t g_s_pos_cfg = {
    .e_type         = PID_POSITION,
    .f_kp           = 5.0f,             /* 需实测整定       */
    .f_ki           = 0.1f,             /* 远小于Kp, 可为0  */
    .f_kd           = 0.05f,            /* 谨慎添加         */
    .f_dt           = PL_TS,
    .f_out_max      = PL_OMEGA_MAX,
    .f_int_max      = PL_OMEGA_MAX,
    .f_d_rc         = 0.002f,           /* 微分低通, 抑噪    */
    .b_d_on_measure = true
};

//*****************************Static Variables******************************//


/**
 * @brief 初始化位置环.
 *
 * @return position_loop_status_t : 函数执行状态.
 *
 */
position_loop_status_t position_loop_init(void)
{
    (void)pid_init(&g_s_pos_pid, &g_s_pos_cfg);
    return POSITION_LOOP_OK;
}


/**
 * @brief 位置环单次计算: (pos_ref, pos_fb) → omega_ref.
 *
 * Steps:
 *  1. 参数校验.
 *  2. 单PI(或PID)计算, 输出速度指令.
 *
 * @param[in]  pos_ref     : 目标位置 (rad, 机械角或编码器计数).
 * @param[in]  pos_fb      : 反馈位置 (与pos_ref同单位).
 * @param[out] p_omega_ref : 输出速度指令指针 (rad/s), 送速度环.
 *
 * @return position_loop_status_t : 函数执行状态.
 *
 */
position_loop_status_t position_loop_calc(
    const float            pos_ref,
    const float             pos_fb,
    float    * const   p_omega_ref )
{
    if (NULL == p_omega_ref)
    {
        return POSITION_LOOP_ERR_NULL;
    }

    (void)pid_calc(&g_s_pos_pid, pos_ref, pos_fb, p_omega_ref);

    return POSITION_LOOP_OK;
}
