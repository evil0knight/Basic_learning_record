/******************************************************************************
 * Copyright (C) 2026 YA. All Rights Reserved.
 * @file speed_loop.h
 * @par dependencies stdbool.h
 * @author YA
 * @brief 专用速度 PI；机械角速度/rad/s 转换为 Iq/A。
 * Processing flow: init -> calc -> reset。
 * @version V2.0
 * @note 四空格缩进，UTF-8。
 *****************************************************************************/
#ifndef SPEED_LOOP_H
#define SPEED_LOOP_H

//****************************** Includes begin ******************************//
#include <stdbool.h>
//******************************* Includes end *******************************//

//******************************* Enums begin ********************************//
typedef enum
{
    SPEED_LOOP_OK = 0,
    SPEED_LOOP_ERR_PARAM,
    SPEED_LOOP_ERR_STATE
} speed_loop_status_t;
//******************************** Enums end *********************************//

//****************************** Structs begin *******************************//
typedef struct
{
    float f_kp;
    float f_ki;
    float f_dt;       /* 实际调度周期/s。 */
    float f_out_max;  /* 输出对称限幅/A，必须为正。 */
    float f_int_max;  /* 积分对称限幅/A，0 禁用积分限幅。 */
} st_speed_loop_cfg_t;

typedef struct
{
    float f_kp;
    float f_i_gain;
    float f_out_max;
    float f_int_max;
    float f_integral;
    bool b_init;
} st_speed_loop_t;
//******************************* Structs end ********************************//

//************************ Function Prototypes begin *************************//
/** @brief 校验配置并预计算矩形积分系数，失败不修改对象。 */
speed_loop_status_t speed_loop_init(st_speed_loop_t *p_loop,
    const st_speed_loop_cfg_t *p_cfg);
/** @brief 清除积分，保留配置。 */
speed_loop_status_t speed_loop_reset(st_speed_loop_t *p_loop);
/** @brief 计算 Iq 指令；失败不修改历史及输出。 */
speed_loop_status_t speed_loop_calc(st_speed_loop_t *p_loop,
    float f_ref, float f_feedback, float *p_iq);
//************************* Function Prototypes end **************************//
#endif
