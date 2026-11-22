/******************************************************************************
 * Copyright (C) 2026 YA. All Rights Reserved.
 * @file current_loop.h
 * @author YA
 * @brief 双轴 PI 电流环，参数和状态归调用者所有。
 * @note 四空格缩进，UTF-8；实时接口的指针由调用者保证有效。
 *****************************************************************************/
#ifndef CURRENT_LOOP_H
#define CURRENT_LOOP_H
//****************************** Includes begin ******************************//
#include <stdbool.h>
//******************************* Includes end *******************************//
//******************************* Enums begin ********************************//
typedef enum
{
    CURRENT_LOOP_OK = 0,
    CURRENT_LOOP_ERR_PARAM,
    CURRENT_LOOP_ERR_STATE
} current_loop_status_t;
//******************************** Enums end *********************************//
//****************************** Structs begin *******************************//
typedef struct
{
    float f_kp;                          /* 比例增益。 */
    float f_ki;                          /* 积分增益。 */
    float f_dt;                          /* 控制周期/s，须大于 0。 */
    float f_out_max;                     /* 单轴输出限幅/V，0 关闭。 */
    float f_int_max;                     /* 积分限幅/V，0 关闭。 */
} st_current_pi_cfg_t;

typedef struct
{
    float f_kp;                          /* 比例增益。 */
    float f_i_gain;                      /* 初始化预计算的积分系数。 */
    float f_out_max;                     /* 单轴输出限幅/V，0 关闭。 */
    float f_int_max;                     /* 积分限幅/V，0 关闭。 */
    float f_integral;                    /* 积分状态。 */
    float f_err_1;                       /* 前次误差。 */
} st_current_pi_t;

typedef struct
{
    st_current_pi_t st_d_pi;              /* d 轴 PI 状态。 */
    st_current_pi_t st_q_pi;              /* q 轴 PI 状态。 */
    float f_voltage_limit_v;             /* 电压矢量幅值上限/V。 */
    float f_voltage_limit_sq;            /* 电压矢量幅值上限的平方。 */
    bool b_init;                         /* 已初始化。 */
    bool b_has_sample;                   /* 已提交首个有效样本。 */
} st_current_loop_t;
//******************************* Structs end ********************************//
//************************ Function Prototypes begin *************************//
/** @brief 校验并初始化 PI；不填充示例参数，不启用解耦前馈。 */
current_loop_status_t current_loop_init(st_current_loop_t *p_loop,
    const st_current_pi_cfg_t *p_d_cfg,
    const st_current_pi_cfg_t *p_q_cfg, float f_voltage_limit_v);
/** @brief 清空两轴积分和历史，保留配置及预计算系数。 */
current_loop_status_t current_loop_reset(st_current_loop_t *p_loop);
/** @brief 计算 d/q 电压并做矢量限幅、抗积分饱和反馈。 */
/* 输出地址须与运行状态独立；失败时状态与输出均不变。 */
current_loop_status_t current_loop_calc(st_current_loop_t *p_loop,
    float f_id_ref, float f_iq_ref, float f_id, float f_iq,
    float *p_vd, float *p_vq);
//************************* Function Prototypes end *************************//
#endif
