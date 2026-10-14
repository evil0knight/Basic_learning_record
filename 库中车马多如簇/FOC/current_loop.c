/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file current_loop.c
 *
 * @par dependencies
 * - PID.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief FOC电流环模块: 两个独立PI控制 Id/Iq, 带反电动势解耦.
 *
 * ============================================================================
 *                              调用方式
 * ============================================================================
 *
 *   // 1. 启动时(main里一次)
 *   current_loop_init();
 *
 *   // 2. PWM中断里, 做完Clarke+Park拿到id/iq后调用
 *   float vd, vq;
 *   current_loop_calc(id_ref, iq_ref, id, iq, omega_e, &vd, &vq);
 *   // 把vd/vq送去反Park→SVPWM
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

#define CL_V_BUS       (24.0f)              /* 母线电压 (V)             */
#define CL_V_MAX       (CL_V_BUS * 0.577f)  /* Vbus/√3, 线性调制区上限 */
#define CL_L_D         (0.0005f)            /* d轴电感 (H)              */
#define CL_L_Q         (0.0005f)            /* q轴电感 (H)              */
#define CL_PSI_F       (0.01f)              /* 永磁体磁链 (Wb)          */

//*********************************Defines***********************************//


//*******************************Enumerations********************************//

typedef enum
{
    CURRENT_LOOP_OK       = 0,
    CURRENT_LOOP_ERR_NULL = 1
} current_loop_status_t;

//*******************************Enumerations********************************//


//*****************************Static Variables******************************//

static pid_t g_s_id_pid;
static pid_t g_s_iq_pid;

/* d/q 共用一份配置,磁饱和严重再分开 */
static const pid_cfg_t g_s_i_cfg = {
    .e_type         = PID_POSITION,
    .f_kp           = 10.0f,            /* ≈ L/Ts, 需实测整定 */
    .f_ki           = 500.0f,           /* ≈ R/Ts             */
    .f_dt           = 0.0001f,          /* 10kHz电流环         */
    .f_out_max      = CL_V_MAX,
    .f_int_max      = CL_V_MAX,
    .b_d_on_measure = true
};

//*****************************Static Variables******************************//


/**
 * @brief 初始化电流环(两个PI实例).
 *
 * @return current_loop_status_t : 函数执行状态.
 *
 */
current_loop_status_t current_loop_init(void)
{
    (void)pid_init(&g_s_id_pid, &g_s_i_cfg);
    (void)pid_init(&g_s_iq_pid, &g_s_i_cfg);
    return CURRENT_LOOP_OK;
}


/**
 * @brief 电流环单次计算: (Iref, Ifb, ω) → (Vd, Vq).
 *
 * Steps:
 *  1. 参数校验.
 *  2. 两个PI分别对Id, Iq做计算.
 *  3. 反电动势解耦前馈:
 *       Vd -= ω·Lq·Iq
 *       Vq += ω·Ld·Id + ω·ψf
 *
 * @param[in]  id_ref  : d轴电流目标值 (A).
 * @param[in]  iq_ref  : q轴电流目标值 (A).
 * @param[in]  id_fb   : d轴电流反馈值 (A).
 * @param[in]  iq_fb   : q轴电流反馈值 (A).
 * @param[in]  omega_e : 转子电角速度 (rad/s).
 * @param[out] p_vd    : 输出Vd指针 (V).
 * @param[out] p_vq    : 输出Vq指针 (V).
 *
 * @return current_loop_status_t : 函数执行状态.
 *
 */
current_loop_status_t current_loop_calc(
    const float          id_ref,
    const float          iq_ref,
    const float           id_fb,
    const float           iq_fb,
    const float         omega_e,
    float    * const       p_vd,
    float    * const       p_vq )
{
    float vd;
    float vq;

    if ((NULL == p_vd) || (NULL == p_vq))
    {
        return CURRENT_LOOP_ERR_NULL;
    }

    vd = 0.0f;
    vq = 0.0f;

    /* 两个PI本职工作 */
    (void)pid_calc(&g_s_id_pid, id_ref, id_fb, &vd);
    (void)pid_calc(&g_s_iq_pid, iq_ref, iq_fb, &vq);

    /* 反电动势解耦前馈 */
    vd -= omega_e * CL_L_Q * iq_fb;
    vq += (omega_e * CL_L_D * id_fb) + (omega_e * CL_PSI_F);

    *p_vd = vd;
    *p_vq = vq;

    return CURRENT_LOOP_OK;
}
