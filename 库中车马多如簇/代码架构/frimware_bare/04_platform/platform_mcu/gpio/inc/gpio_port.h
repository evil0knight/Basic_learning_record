/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file gpio_port.h
 *
 * @par dependencies
 * - stdbool.h
 * - stdint.h
 * - main.h
 *
 * @author YA
 *
 * @brief 头文件，用于实现本模块接口及相关功能。
 *
 * Processing flow:
 *
 * 参数检查 -> 执行模块操作 -> 返回处理结果。
 *
 * @version V1.0 2026-10-24
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef GPIO_PORT_H
#define GPIO_PORT_H

//******************************** Includes *********************************//
#include <stdbool.h>
#include <stdint.h>

#include "main.h"
//******************************** Includes *********************************//

//******************************** Typedefs *********************************//
typedef enum
{
    CORE_GPIO_OK = 0,
    CORE_GPIO_ERROR,
    CORE_GPIO_ERR_ARGUMENT,
    CORE_GPIO_ERR_NOT_CONFIGURED
} en_core_gpio_status_t;

typedef enum
{
    CORE_GPIO_PIN_RESET = 0,
    CORE_GPIO_PIN_SET
} en_core_gpio_pin_state_t;

typedef enum
{
    CORE_GPIO_DISPLAY_RESET = 0,
    CORE_GPIO_DISPLAY_DC,
    CORE_GPIO_DISPLAY_BACKLIGHT,
    CORE_GPIO_FLASH_WP,
    CORE_GPIO_LED_EN,
    CORE_GPIO_KEY0,
    CORE_GPIO_SENSOR_INT,
    CORE_GPIO_PIN_MAX
} en_core_gpio_pin_t;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    en_core_gpio_pin_state_t default_state;
    bool initialize_output;
} st_core_gpio_config_t;
//******************************** Typedefs *********************************//

//******************************** Defines **********************************//

//******************************** Defines **********************************//

//******************************** APIs *************************************//
/**
 * @brief 为已配置的输出引脚应用默认电平。
 * @return GPIO 操作状态。
 */
en_core_gpio_status_t core_gpio_init(void);

/**
 * @brief 设置逻辑 GPIO 引脚电平。
 * @param[in] id 逻辑 GPIO 引脚标识。
 * @param[in] state 输出电平。
 * @return GPIO 操作状态。
 */
en_core_gpio_status_t core_gpio_write_pin(en_core_gpio_pin_t id,
                                          en_core_gpio_pin_state_t state);

/**
 * @brief 读取逻辑 GPIO 引脚电平。
 * @param[in] id 逻辑 GPIO 引脚标识。
 * @return 引脚电平；标识无效或引脚未配置时返回 CORE_GPIO_PIN_RESET。
 */
en_core_gpio_pin_state_t core_gpio_read_pin(en_core_gpio_pin_t id);

/**
 * @brief 翻转逻辑 GPIO 输出电平。
 * @param[in] id 逻辑 GPIO 引脚标识。
 * @return GPIO 操作状态。
 */
en_core_gpio_status_t core_gpio_toggle_pin(en_core_gpio_pin_t id);

/**
 * @brief 直接设置物理 GPIO 电平，无需逻辑配置。
 * @param[in] port GPIO 外设端口。
 * @param[in] pin GPIO 引脚掩码。
 * @param[in] state 输出电平。
 * @return GPIO 操作状态。
 */
en_core_gpio_status_t core_gpio_write_pin_direct(GPIO_TypeDef *port,
                                                 uint16_t pin,
                                                 GPIO_PinState state);

/**
 * @brief 直接读取物理 GPIO 电平，无需逻辑配置。
 * @param[in] port GPIO 外设端口。
 * @param[in] pin GPIO 引脚掩码。
 * @return 引脚电平；端口为空或引脚掩码为零时返回 GPIO_PIN_RESET。
 */
GPIO_PinState core_gpio_read_pin_direct(GPIO_TypeDef *port, uint16_t pin);

/**
 * @brief 直接翻转物理 GPIO 电平，无需逻辑配置。
 * @param[in] port GPIO 外设端口。
 * @param[in] pin GPIO 引脚掩码。
 * @return GPIO 操作状态。
 */
en_core_gpio_status_t core_gpio_toggle_pin_direct(GPIO_TypeDef *port,
                                                  uint16_t pin);

/**
 * @brief 由 HAL 的 port + pin 掩码反向查找配置表中的逻辑 ID。
 *        供 EXTI 回调把 HAL 给的 GPIO_Pin 翻译成已登记的逻辑 ID。
 * @param[in] port GPIO 外设端口；传 NULL 时忽略端口比对，只按 pin 匹配。
 * @param[in] pin  GPIO 引脚掩码。
 * @param[out] p_id 命中时写入对应逻辑 ID。
 * @return GPIO 操作状态；未命中返回 CORE_GPIO_ERR_NOT_CONFIGURED。
 */
en_core_gpio_status_t core_gpio_find_by_hal_pin(GPIO_TypeDef *port,
                                                uint16_t pin,
                                                en_core_gpio_pin_t *p_id);
//******************************** APIs *************************************//

#endif
