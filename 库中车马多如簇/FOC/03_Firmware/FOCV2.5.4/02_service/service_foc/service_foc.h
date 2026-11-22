/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file service_foc.h
 *
 * @par dependencies
 * - ../../01_app/app_control/app_control.h
 * - ../../04_platform/platform_mcu/adc/inc/adc_port.h
 * - ../../04_platform/platform_bsp/current/wrapper/bsp_wrapper_current.h
 *
 * @author YA
 *
 * @brief FOC 服务初始化与电流控制周期接口。
 *
 * Processing flow:
 *
 * 初始化 -> 注册 ADC Injected 入口 -> 电流读取 -> 编码器读取 -> FOC 控制。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef SERVICE_FOC_H
#define SERVICE_FOC_H

//****************************** Includes begin ******************************//
#include "../../01_app/app_control/app_control.h"
#include "../../04_platform/platform_mcu/adc/inc/adc_port.h"
#include "../../04_platform/platform_bsp/current/wrapper/bsp_wrapper_current.h"
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 初始化编码器、电流服务和 ADC Injected 控制链。
 *
 * Steps:
 *  1. 初始化电机编码器。
 *  2. 初始化电流服务。
 *  3. 初始化期间保持 FOC 控制回调解除注册。
 *  4. 先启动 ADC2 从机，再启动 ADC1 主机的 Injected 中断采样。
 *  5. 启动 HRTIM Master 计数，产生 ADC 外部触发事件。
 *  6. 保持输出关闭，阻塞确认静止并校准电流；阈值未确认只诊断。
 *  7. 仅启动电机 1 的 Timer A/B/C 计数，PWM 输出保持关闭。
 *  8. 最后注册 ADC1 控制回调，开始执行 FOC 周期。
 *
 * @return app_status_t : 初始化结果。
 * @note HRTIM 须事先完成硬件初始化，ADC 须事先完成内部校准。
 */
app_status_t service_foc_init(void);

//************************* Function Prototypes end **************************//

#endif /* SERVICE_FOC_H */
