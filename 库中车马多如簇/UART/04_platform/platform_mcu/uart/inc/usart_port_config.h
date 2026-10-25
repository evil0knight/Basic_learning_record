#ifndef USART_PORT_CONFIG_H
#define USART_PORT_CONFIG_H

#include "usart_port.h"

/*
 * USART 槽位表：每个实例填 HAL 句柄、波特率、默认超时。
 * 移植时用目标板资源替换表项。
 */

static const st_usart_config_t g_usart_configs[CORE_USART_MAX] =
{
    [CORE_USART1] = {
        .handle = &huart1,
        .baudrate = 115200U,
        .timeout = CORE_USART_DEFAULT_TIMEOUT
    }
};

#endif /* USART_PORT_CONFIG_H */
