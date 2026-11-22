/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.h
 *
 * @par dependencies
 * - adc.h
 *
 * @author YA
 *
 * @brief 声明 ADC HAL 转换完成和错误回调入口。
 *
 * Processing flow:
 *
 * HAL 回调 -> Platform 检查句柄并分发事件。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef ADC_USER_ISR_HANDLERS_H
#define ADC_USER_ISR_HANDLERS_H

//****************************** Includes begin ******************************//
#include "adc.h"
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 将 HAL ADC 转换完成事件转发给 Platform。
 *
 * Steps:
 *  1. 调用 core_adc_irq_conv_complete，由 Platform 检查句柄并分发事件。
 *
 * @param[in] hadc : 触发事件的 HAL ADC 句柄。
 * @return void : HAL 回调固定签名。
 * @note 在中断上下文执行，不在入口中处理采样业务。
 *
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);

/**
 * @brief 将 HAL ADC 错误事件转发给 Platform。
 *
 * Steps:
 *  1. 调用 core_adc_irq_error，由 Platform 检查句柄并分发错误。
 *
 * @param[in] hadc : 发生错误的 HAL ADC 句柄。
 * @return void : HAL 回调固定签名。
 * @note 在中断上下文执行。
 *
 */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc);

//************************* Function Prototypes end **************************//

#endif
//******************************* End of File ********************************//
