/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file gpio_port_config.h
 *
 * @par dependencies
 * - gpio_port.h
 *
 * @author YA
 *
 * @brief 配置逻辑 GPIO 对应的板级引脚和默认电平。
 *
 * Processing flow:
 *
 * 仅由 gpio_port.c 包含，移植时填写目标板资源。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef GPIO_PORT_CONFIG_H
#define GPIO_PORT_CONFIG_H

#include "gpio_port.h"

/*
 * 未使用的逻辑引脚保持零初始化，访问时返回未配置错误。
 * 在下表中按逻辑 ID 添加配置；物理宏由目标工程的 main.h 提供。
 * 按键这类输入引脚只填 port / pin，default_state 和 initialize_output
 * 可保持零（它们只影响输出）。
 * 例如：
 * [CORE_GPIO_DISPLAY_DC] = {
 *     .port = LCD_DC_GPIO_Port,
 *     .pin = LCD_DC_Pin,
 *     .default_state = CORE_GPIO_PIN_RESET,
 *     .initialize_output = true
 * },
 * [CORE_GPIO_KEY0] = {
 *     .port = KEY0_GPIO_Port,
 *     .pin  = KEY0_Pin
 * }
 */
static const st_core_gpio_config_t g_gpio_configs[CORE_GPIO_PIN_MAX] = {0};

#endif /* GPIO_PORT_CONFIG_H */
