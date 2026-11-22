/******************************************************************************
 * Copyright (C) 2026 YA. All Rights Reserved.
 * @file position_loop.h
 * @par dependencies stdbool.h
 * @author YA
 * @brief 单圈最短路径位置控制，输出机械速度/rad/s。
 * Processing flow: init -> set_position_ref -> calc -> reset。
 * @version V2.0
 * @note 四空格缩进，UTF-8。与中断共享对象时，调用方保护写操作。
 *****************************************************************************/
#ifndef POSITION_LOOP_H
#define POSITION_LOOP_H

//****************************** Includes begin ******************************//
#include <stdbool.h>
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
#define POSITION_LOOP_TWO_PI (6.2831853071795864769f)
#define POSITION_LOOP_PI     (3.1415926535897932385f)
//******************************* Defines end ********************************//

//******************************* Enums begin ********************************//
typedef enum
{
    POSITION_LOOP_OK = 0,
    POSITION_LOOP_ERR_PARAM,
    POSITION_LOOP_ERR_STATE
} position_loop_status_t;
//******************************** Enums end *********************************//

//****************************** Structs begin *******************************//
typedef struct
{
    float f_kp;
    float f_ki;
    float f_kd;
    float f_dt;
    float f_out_max; /* 机械速度对称限幅/rad/s，必须为正。 */
    float f_int_max;
    float f_d_rc;    /* 微分滤波时间常数/s，0 不滤波。 */
} st_position_loop_cfg_t;

typedef struct
{
    float f_kp;
    float f_i_gain;
    float f_d_gain;
    float f_d_alpha;
    float f_out_max;
    float f_int_max;
    float f_integral;
    float f_d_prev;
    float f_feedback_prev;
    float f_position_ref_rad;
    bool b_init;
    bool b_has_sample;
    bool b_target_valid; /* 显式命令标记；自动保持目标不置位。 */
} st_position_loop_t;
//******************************* Structs end ********************************//

//************************ Function Prototypes begin *************************//
/** @brief 校验配置并预计算积分、微分及滤波系数。 */
position_loop_status_t position_loop_init(st_position_loop_t *p_loop,
    const st_position_loop_cfg_t *p_cfg);
/** @brief 清除历史，保留配置和目标。 */
position_loop_status_t position_loop_reset(st_position_loop_t *p_loop);
/**
 * @brief 保存 [0, 2π) 的机械位置目标/rad。
 * @note 与控制中断并发时，调用方需在短临界区内调用。
 */
position_loop_status_t position_loop_set_position_ref(
    st_position_loop_t *p_loop, float f_position_ref_rad);
/** @brief 使用已保存目标及 [0, 2π) 反馈计算速度，半圈取负向。 */
position_loop_status_t position_loop_calc(st_position_loop_t *p_loop,
    float f_feedback_rad, float *p_speed_ref);
//************************* Function Prototypes end **************************//
#endif
