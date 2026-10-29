/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file usart_port_config.h
 *
 * @par dependencies
 * - usart_port.h
 *
 * @author YA
 *
 * @brief USART 实例配置表，绑定 HAL 句柄、波特率及默认超时。
 *
 * Processing flow:
 *
 * 编译期填充静态表 -> USART Port 按逻辑实例访问对应配置。
 *
 * @version V1.0 2026-10-27
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef USART_PORT_CONFIG_H
#define USART_PORT_CONFIG_H

//****************************** Includes begin ******************************//
#include "usart_port.h"
//******************************* Includes end *******************************//

//************************** Static Variables begin **************************//
/**
 * @brief USART 逻辑实例配置表。
 *
 * 移植时按目标板资源修改句柄、波特率及默认超时。
 */
static const st_usart_config_t g_usart_configs[CORE_USART_MAX] =
{
    // [CORE_USART1] =
    // {
    //     .p_handle = &huart1,
    //     .baudrate = 115200U,
    //     .timeout = CORE_USART_DEFAULT_TIMEOUT
    // }
};
//*************************** Static Variables end ***************************//

#endif /* USART_PORT_CONFIG_H */
