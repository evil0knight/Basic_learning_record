/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file service_foc_motor1.h
 *
 * @par dependencies
 * - service_foc_motor.h
 * - ../../01_app/app_control/app_control.h
 *
 * @author YA
 *
 * @brief 电机 1 独立控制接口。
 *
 * Processing flow:
 *
 * 由对应电机控制中断调用独立入口；硬件注册由初始化模块负责。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef SERVICE_FOC_MOTOR1_H
#define SERVICE_FOC_MOTOR1_H

//****************************** Includes begin ******************************//
#include "service_foc_motor.h"
#include "../../04_platform/platform_bsp/current/wrapper/bsp_wrapper_current.h"
#include "../../01_app/app_control/app_control.h"
//******************************* Includes end *******************************//

//************************ Extern Declarations begin *************************//
//************************* Extern Declarations end **************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 获取电机状态地址，供初始化、标定及位置给定设置使用。
 * @note 用短临界区调用 position_loop_set_position_ref 设置机械弧度目标；
 *       其他配置修改仍须暂停控制回调。
 */
app_status_t service_foc_motor1_get_state(st_service_foc_motor_t **pp_motor);

void service_foc_motor1_foc_task(en_core_adc_instance_t instance);

/** @brief 20kHz 每拍调用，内部执行 5kHz DMA、1kHz 位置环和速度环。 */
app_status_t service_foc_motor1_5khz_task(void);

void service_foc_motor1_encoder_task(void);

void service_foc_motor1_current_task(void);

void service_foc_motor1_current_loop_task(void);

app_status_t service_foc_motor1_pwm_enable(uint8_t u8_enable);

//************************* Function Prototypes end **************************//

#endif /* SERVICE_FOC_MOTOR1_H */
