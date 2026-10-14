/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file pll.c
 *
 * @par dependencies
 * - sin_lut.h
 * - PID.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief FOC位置观测器PLL锁相环: 把反电动势相位误差锁进PI, 吐出平滑电角度与电角速度.
 *
 * ============================================================================
 *                              调用方式
 * ============================================================================
 *
 *   公式:
 *     Park:   ê_d =  e_α·cos(θ̂) + e_β·sin(θ̂)
 *     Err:    err = 0 - ê_d                     // ê_d → 0 ⇒ 角度对齐
 *     PI:     ω̂_e = PI(target=0, measure=ê_d)
 *     积分:   θ̂(k) = wrap( θ̂(k-1) + ω̂_e·Ts )
 *
 *   sin LUT以const常量形式预嵌入Flash (sin_lut.c), 开机即可用, 无需init.
 *
 *   // 1. 启动时(main里一次)
 *   pll_init();
 *
 *   // 2. 开环启动结束/切换到观测器时, 对齐初始角度避免大幅跳变
 *   pll_set_angle(theta_open_loop);
 *
 *   // 3. PWM中断里, 做完SMO/观测器拿到 e_alpha, e_beta 后调用
 *   float theta_hat, omega_hat;
 *   pll_calc(e_alpha, e_beta, &theta_hat, &omega_hat);
 *   // theta_hat 送去 Park / 反Park, omega_hat 给电流环前馈 / 速度环.
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
#include "sin_lut.h"

//********************************Includes***********************************//


//*********************************Defines***********************************//

#define PLL_TS           (0.0001f)              /* 10kHz, 与电流环同步     */
#define PLL_KP           (50.0f)                /* 比例, 需实测整定         */
#define PLL_KI           (1000.0f)              /* 积分, 需实测整定         */
#define PLL_OMEGA_MAX    (6283.0f)              /* 角速度限幅 ≈ 1kHz电频率  */
#define PLL_TWO_PI       (6.28318530718f)       /* 2π                       */

//*********************************Defines***********************************//


//*******************************Enumerations********************************//

typedef enum
{
    PLL_OK           = 0,   /* 计算成功       */
    PLL_ERR_NULL     = 1,   /* 空指针         */
    PLL_ERR_LUT      = 2    /* LUT查表失败    */
} pll_status_t;

//*******************************Enumerations********************************//


//*****************************Static Variables******************************//

static pid_t  g_s_pll_pid;                      /* 环路PI实例      */
static float  g_s_theta_hat;                    /* 估计电角度 (rad)*/
static float  g_s_omega_hat;                    /* 估计电角速度    */

static const pid_cfg_t g_s_pll_cfg = {
    .e_type         = PID_POSITION,
    .f_kp           = PLL_KP,
    .f_ki           = PLL_KI,
    .f_dt           = PLL_TS,
    .f_out_max      = PLL_OMEGA_MAX,
    .f_int_max      = PLL_OMEGA_MAX,
    .b_d_on_measure = false
};

//*****************************Static Variables******************************//


/**
 * @brief 初始化PLL(PI实例 + 内部状态).
 *
 * Steps:
 *  1. 初始化环路PI控制器.
 *  2. 清零估计角度与角速度.
 *
 * @return pll_status_t : 函数执行状态.
 *
 */
pll_status_t pll_init(void)
{
    (void)pid_init(&g_s_pll_pid, &g_s_pll_cfg);

    g_s_theta_hat = 0.0f;
    g_s_omega_hat = 0.0f;

    return PLL_OK;
}


/**
 * @brief 复位PLL历史, 保留PI配置.
 *
 * 用于电机停转、换向或从开环切闭环前清掉旧积分, 避免历史污染.
 *
 * @return pll_status_t : 函数执行状态.
 *
 */
pll_status_t pll_reset(void)
{
    (void)pid_reset(&g_s_pll_pid);

    g_s_theta_hat = 0.0f;
    g_s_omega_hat = 0.0f;

    return PLL_OK;
}


/**
 * @brief 强制对齐PLL的估计角度.
 *
 * 开环启动阶段用的是I/f或V/f给出的"假角度", 切换到PLL闭环前先把
 * 内部 θ̂ 塞成当前的开环角度, 避免切换瞬间角度突跳.
 *
 * @param[in] f_theta : 要塞入的初始电角度 (rad).
 *
 * @return pll_status_t : 函数执行状态.
 *
 */
pll_status_t pll_set_angle(const float f_theta)
{
    g_s_theta_hat = f_theta;

    return PLL_OK;
}


/**
 * @brief PLL单次计算: (e_α, e_β) → (θ̂, ω̂).
 *
 * Steps:
 *  1. 参数校验, 空指针直接返回错误.
 *  2. 用上一拍 θ̂ 查一次 sin/cos.
 *  3. Park投影得 ê_d (ê_q 不用, 省一次乘加).
 *  4. 以 target=0, measure=ê_d 喂PI, 输出即角速度估计 ω̂_e.
 *     内部误差 err = 0 - ê_d, 符号自洽:
 *       θ̂落后 → ê_d 为非零 → PI输出调整 ω̂_e → 加速/减速对齐.
 *  5. 角速度积分推进角度, wrap到 [0, 2π) 防数值发散.
 *  6. 更新内部状态并输出.
 *
 * @param[in]  e_alpha     : α轴反电动势 (V).
 * @param[in]  e_beta      : β轴反电动势 (V).
 * @param[out] p_theta_hat : 估计电角度输出指针 (rad, [0, 2π)).
 * @param[out] p_omega_hat : 估计电角速度输出指针 (rad/s).
 *
 * @return pll_status_t : 函数执行状态.
 *
 */
pll_status_t pll_calc(
    const float            e_alpha,
    const float             e_beta,
    float    * const   p_theta_hat,
    float    * const   p_omega_hat )
{
    float            sin_theta;
    float            cos_theta;
    float            e_d;
    float            omega_new;
    float            theta_new;
    sin_lut_status_t lut_s;

    /* 1. 参数校验 */
    if ((NULL == p_theta_hat) || (NULL == p_omega_hat))
    {
        return PLL_ERR_NULL;
    }

    /* 2. 用上一拍 θ̂ 查 sin/cos */
    lut_s = sin_lut_sincos(g_s_theta_hat, &sin_theta, &cos_theta);
    if (SIN_LUT_OK != lut_s)
    {
        return PLL_ERR_LUT;
    }

    /* 3. Park投影, 只取 ê_d */
    /*    ê_d = e_α·cos(θ̂) + e_β·sin(θ̂) */
    e_d = (e_alpha * cos_theta) + (e_beta * sin_theta);

    /* 4. PI: target=0, measure=ê_d, 输出就是角速度估计 */
    (void)pid_calc(&g_s_pll_pid, 0.0f, e_d, &omega_new);

    /* 5. 角速度积分推进角度, 并wrap到 [0, 2π) */
    theta_new = g_s_theta_hat + (omega_new * PLL_TS);
    while (PLL_TWO_PI <= theta_new)
    {
        theta_new -= PLL_TWO_PI;
    }
    while (0.0f > theta_new)
    {
        theta_new += PLL_TWO_PI;
    }

    /* 6. 更新内部状态并输出 */
    g_s_theta_hat = theta_new;
    g_s_omega_hat = omega_new;

    *p_theta_hat  = theta_new;
    *p_omega_hat  = omega_new;

    return PLL_OK;
}
