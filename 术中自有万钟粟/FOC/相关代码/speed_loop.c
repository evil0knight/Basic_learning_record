/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file speed_loop.c
 *
 * @par dependencies
 * - PID.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief FOC速度环模块: 单PI把速度误差转成q轴电流指令Iq_ref.
 *
 * ============================================================================
 *                              调用方式
 * ============================================================================
 *
 *   // 1. 启动时(main里一次)
 *   speed_loop_init();
 *
 *   // 2. 速度环定时任务里(2kHz, 电流环的1/5~1/10), 拿到目标/反馈速度后调用
 *   float iq_ref;
 *   speed_loop_calc(omega_ref, omega_fb, &iq_ref);
 *   // 把iq_ref送给电流环当参考
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

#define SL_TS          (0.0005f)        /* 2kHz速度环, 电流环的1/5          */
#define SL_IQ_MAX      (20.0f)          /* q轴电流上限 (A), 对应最大转矩    */
#define SL_VSI_A       (10.0f)          /* 变速积分过渡宽度, 大误差减积分   */
#define SL_VSI_B       (2.0f)           /* 变速积分全积分阈值, 小误差满积分 */

//*********************************Defines***********************************//


//*******************************Enumerations********************************//

typedef enum
{
    SPEED_LOOP_OK       = 0,
    SPEED_LOOP_ERR_NULL = 1
} speed_loop_status_t;

//*******************************Enumerations********************************//


//*****************************Static Variables******************************//

static pid_t g_s_spd_pid;

/* 速度环典型做PI,不加D(反馈本身带噪).
 * 启用变速积分抑制大误差时的积分饱和,比简单积分限幅更干净. */
static const pid_cfg_t g_s_spd_cfg = {
    .e_type         = PID_POSITION,
    .f_kp           = 0.05f,            /* 需实测整定, ω·J/(1.5·Kt) 为理论值 */
    .f_ki           = 2.0f,             /* Kp·ωc/5, 需实测整定               */
    .f_dt           = SL_TS,
    .f_out_max      = SL_IQ_MAX,
    .f_int_max      = SL_IQ_MAX,
    .f_vsi_a        = SL_VSI_A,
    .f_vsi_b        = SL_VSI_B
};

//*****************************Static Variables******************************//


/**
 * @brief 初始化速度环.
 *
 * @return speed_loop_status_t : 函数执行状态.
 *
 */
speed_loop_status_t speed_loop_init(void)
{
    (void)pid_init(&g_s_spd_pid, &g_s_spd_cfg);
    return SPEED_LOOP_OK;
}


/**
 * @brief 速度环单次计算: (omega_ref, omega_fb) → iq_ref.
 *
 * Steps:
 *  1. 参数校验.
 *  2. 单PI计算, 输出q轴电流指令.
 *
 * @param[in]  omega_ref : 目标电角速度 (rad/s).
 * @param[in]  omega_fb  : 反馈电角速度 (rad/s).
 * @param[out] p_iq_ref  : 输出q轴电流指令指针 (A), 送电流环.
 *
 * @return speed_loop_status_t : 函数执行状态.
 *
 */
speed_loop_status_t speed_loop_calc(
    const float          omega_ref,
    const float           omega_fb,
    float    * const      p_iq_ref )
{
    if (NULL == p_iq_ref)
    {
        return SPEED_LOOP_ERR_NULL;
    }

    (void)pid_calc(&g_s_spd_pid, omega_ref, omega_fb, p_iq_ref);

    return SPEED_LOOP_OK;
}
