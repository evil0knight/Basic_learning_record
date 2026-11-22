/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.c
 *
 * @par dependencies
 * - user_isr_handlers.h
 * - adc_port.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief ADC HAL 回调入口，仅负责转发 Platform 事件。
 *
 * Processing flow:
 *
 * HAL 回调 -> core_adc_irq_* -> Service 注册的通知回调。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//
//****************************** Includes begin ******************************//
#include "user_isr_handlers.h"

#include "adc_port.h"

#include <stddef.h>
//******************************* Includes end *******************************//
//

//

//**************************** ISR Handlers begin ****************************//
/**
 * @brief 将 HAL ADC 转换完成事件转发给 Platform。
 *
 * Steps:
 *  1. 检查 HAL ADC 句柄，空指针时直接返回。
 *  2. 调用 core_adc_irq_conv_complete，由 Platform 分发事件。
 *
 * @param[in] hadc : 触发事件的 HAL ADC 句柄。
 * @return void : HAL 回调固定签名。
 * @note 在中断上下文执行，不在入口中处理采样业务。
 *
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == hadc)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_adc_irq_conv_complete(hadc);
}

/**
 * @brief 将 HAL ADC 错误事件转发给 Platform。
 *
 * Steps:
 *  1. 检查 HAL ADC 句柄，空指针时直接返回。
 *  2. 调用 core_adc_irq_error，由 Platform 分发错误。
 *
 * @param[in] hadc : 发生错误的 HAL ADC 句柄。
 * @return void : HAL 回调固定签名。
 * @note 在中断上下文执行。
 *
 */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == hadc)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_adc_irq_error(hadc);
}
//

//***************************** ISR Handlers end *****************************//

//******************************* End of File ********************************//
