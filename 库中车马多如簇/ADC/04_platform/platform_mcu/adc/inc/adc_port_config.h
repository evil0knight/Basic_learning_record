/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file adc_port_config.h
 *
 * @par dependencies
 * - adc_port.h
 *
 * @author YA
 *
 * @brief 配置 ADC 实例句柄及轮询超时。
 *
 * Processing flow:
 *
 * 编译期配置表 -> Platform 按逻辑实例访问 HAL 句柄。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef ADC_PORT_CONFIG_H
#define ADC_PORT_CONFIG_H

//****************************** Includes begin ******************************//
#include "adc_port.h"
//******************************* Includes end *******************************//

//***************************** Constants begin ******************************//
/** @brief 实例到 HAL 句柄的静态映射，移植时按目标板资源修改。 */
static const st_adc_config_t g_adc_configs[CORE_ADC_MAX] =
{
    [CORE_ADC1] = {
        .handle = &hadc1,
        .timeout = CORE_ADC_DEFAULT_TIMEOUT
    }
};
//****************************** Constants end *******************************//

#endif

//******************************* End of File ********************************//
