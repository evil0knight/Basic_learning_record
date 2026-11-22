/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file service_foc_motor2.h
 *
 * @par dependencies
 * - service_foc_motor.h
 * - ../../01_app/app_control/app_control.h
 *
 * @author YA
 *
 * @brief 电机 2 独立控制接口。
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

#ifndef SERVICE_FOC_MOTOR2_H
#define SERVICE_FOC_MOTOR2_H

//****************************** Includes begin ******************************//
#include "service_foc_motor.h"
#include "../../01_app/app_control/app_control.h"
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 电机 2 独立 ADC 控制中断的预留入口。
 * @param[in] instance : ADC 实例，后续确定硬件映射后添加检查。
 * @return void : 无返回值。
 * @note 当前不注册硬件回调。
 */
void service_foc_motor2_current_entry(en_core_adc_instance_t instance);

/**
 * @brief 电机 2 控制周期框架，暂不执行采样或输出。
 *
 * Steps:
 *  1. 调用本电机低频任务预留入口。
 *  2. 预留每拍反馈、电流环和 PWM 更新。
 *
 * @return void : 无返回值。
 */
void service_foc_motor2_current_tick(void);

/**
 * @brief 向初始化模块提供本电机私有状态的地址。
 *
 * Steps:
 *  1. 检查输出指针并返回私有状态地址。
 *
 * @param[out] pp_motor : 电机状态地址的输出位置。
 * @return app_status_t : 成功或参数错误。
 * @note 仅供初始化使用，调用时须停用本电机控制中断。
 */
app_status_t service_foc_motor2_get_state(st_service_foc_motor_t **pp_motor);
//************************* Function Prototypes end **************************//

#endif /* SERVICE_FOC_MOTOR2_H */
