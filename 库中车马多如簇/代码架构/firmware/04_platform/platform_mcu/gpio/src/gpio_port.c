/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file gpio_port.c
 *
 * @par dependencies
 * - gpio_port.h
 * - gpio_port_config.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 源文件，用于实现本模块接口及相关功能。
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

#include "gpio_port.h"

#include "gpio_port_config.h"

#include <stddef.h>

/**
 * @brief 为已配置的输出引脚应用默认电平。
 * @return GPIO 操作状态。
 */
en_core_gpio_status_t core_gpio_init(void)
{
    en_core_gpio_pin_t id;

    for (id = (en_core_gpio_pin_t)0; id < CORE_GPIO_PIN_MAX; id++)
    {
        const st_core_gpio_config_t *p_config = &g_gpio_configs[id];

        if ((NULL != p_config->port) && (0U != p_config->pin) &&
            p_config->initialize_output)
        {
            GPIO_PinState state = (p_config->default_state == CORE_GPIO_PIN_SET)
                                      ? GPIO_PIN_SET
                                      : GPIO_PIN_RESET;
            (void)core_gpio_write_pin_direct(p_config->port,
                                             p_config->pin,
                                             state);
        }
    }

    return CORE_GPIO_OK;
}

/**
 * @brief 设置逻辑 GPIO 引脚电平。
 * @param[in] id 逻辑 GPIO 引脚标识。
 * @param[in] state 输出电平。
 * @return GPIO 操作状态。
 */
en_core_gpio_status_t core_gpio_write_pin(en_core_gpio_pin_t id,
                                          en_core_gpio_pin_state_t state)
{
    if (((unsigned int)id >= (unsigned int)CORE_GPIO_PIN_MAX) ||
        ((state != CORE_GPIO_PIN_RESET) && (state != CORE_GPIO_PIN_SET)))
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }

    if ((NULL == g_gpio_configs[id].port) ||
        (0U == g_gpio_configs[id].pin))
    {
        return CORE_GPIO_ERR_NOT_CONFIGURED;
    }

    return core_gpio_write_pin_direct(g_gpio_configs[id].port,
                                      g_gpio_configs[id].pin,
                                      (state == CORE_GPIO_PIN_SET)
                                          ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief 读取逻辑 GPIO 引脚电平。
 * @param[in] id 逻辑 GPIO 引脚标识。
 * @return 引脚电平；标识无效或引脚未配置时返回 CORE_GPIO_PIN_RESET。
 */
en_core_gpio_pin_state_t core_gpio_read_pin(en_core_gpio_pin_t id)
{
    GPIO_PinState raw_state;

    if ((unsigned int)id >= (unsigned int)CORE_GPIO_PIN_MAX)
    {
        return CORE_GPIO_PIN_RESET;
    }

    if ((NULL == g_gpio_configs[id].port) ||
        (0U == g_gpio_configs[id].pin))
    {
        return CORE_GPIO_PIN_RESET;
    }

    raw_state = core_gpio_read_pin_direct(g_gpio_configs[id].port,
                                          g_gpio_configs[id].pin);
    return (GPIO_PIN_SET == raw_state)
               ? CORE_GPIO_PIN_SET : CORE_GPIO_PIN_RESET;
}

/**
 * @brief 翻转逻辑 GPIO 输出电平。
 * @param[in] id 逻辑 GPIO 引脚标识。
 * @return GPIO 操作状态。
 */
en_core_gpio_status_t core_gpio_toggle_pin(en_core_gpio_pin_t id)
{
    if ((unsigned int)id >= (unsigned int)CORE_GPIO_PIN_MAX)
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }

    if ((NULL == g_gpio_configs[id].port) ||
        (0U == g_gpio_configs[id].pin))
    {
        return CORE_GPIO_ERR_NOT_CONFIGURED;
    }

    return core_gpio_toggle_pin_direct(g_gpio_configs[id].port,
                                       g_gpio_configs[id].pin);
}

/**
 * @brief 直接设置物理 GPIO 电平，无需逻辑配置。
 * @param[in] port GPIO 外设端口。
 * @param[in] pin GPIO 引脚掩码。
 * @param[in] state 输出电平。
 * @return GPIO 操作状态。
 */
en_core_gpio_status_t core_gpio_write_pin_direct(GPIO_TypeDef *port,
                                                 uint16_t pin,
                                                 GPIO_PinState state)
{
    if ((port == NULL) || (pin == 0U))
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }

    HAL_GPIO_WritePin(port, pin, state);
    return CORE_GPIO_OK;
}

/**
 * @brief 直接读取物理 GPIO 电平，无需逻辑配置。
 * @param[in] port GPIO 外设端口。
 * @param[in] pin GPIO 引脚掩码。
 * @return 引脚电平；端口为空或引脚掩码为零时返回 GPIO_PIN_RESET。
 */
GPIO_PinState core_gpio_read_pin_direct(GPIO_TypeDef *port, uint16_t pin)
{
    if ((NULL == port) || (0U == pin))
    {
        return GPIO_PIN_RESET;
    }

    return HAL_GPIO_ReadPin(port, pin);
}

/**
 * @brief 直接翻转物理 GPIO 电平，无需逻辑配置。
 * @param[in] port GPIO 外设端口。
 * @param[in] pin GPIO 引脚掩码。
 * @return GPIO 操作状态。
 */
en_core_gpio_status_t core_gpio_toggle_pin_direct(GPIO_TypeDef *port,
                                                  uint16_t pin)
{
    if ((port == NULL) || (pin == 0U))
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }

    HAL_GPIO_TogglePin(port, pin);
    return CORE_GPIO_OK;
}

/**
 * @brief 由 HAL 的 port + pin 掩码反向查找逻辑 ID。
 */
en_core_gpio_status_t core_gpio_find_by_hal_pin(GPIO_TypeDef *port,
                                                uint16_t pin,
                                                en_core_gpio_pin_t *p_id)
{
    en_core_gpio_pin_t id;

    //-------------------------参数检查 begin-------------------------//
    if ((0U == pin) || (NULL == p_id))
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    for (id = (en_core_gpio_pin_t)0; id < CORE_GPIO_PIN_MAX; id++)
    {
        const st_core_gpio_config_t *p_config = &g_gpio_configs[id];
        if ((NULL == p_config->port) || (0U == p_config->pin))
        {
            continue;
        }
        if ((p_config->pin == pin) &&
            ((NULL == port) || (p_config->port == port)))
        {
            *p_id = id;
            return CORE_GPIO_OK;
        }
    }
    return CORE_GPIO_ERR_NOT_CONFIGURED;
}
