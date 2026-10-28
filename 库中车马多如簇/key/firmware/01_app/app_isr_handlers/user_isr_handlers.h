/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc. or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.h
 * @par dependencies
 * - stdint.h
 * @author YA
 * @brief 声明按键 EXTI 回调。
 *
 * Processing flow:
 * HAL EXTI 分发 -> PA0 按键回调 -> 队列投递。
 *
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef KEY_USER_ISR_HANDLERS_H
#define KEY_USER_ISR_HANDLERS_H

#include <stdint.h>

/**
 * @brief 处理 GPIO_PIN_0 的外部中断。
 * @param[in] u16_gpio_pin HAL 传入的引脚掩码。
 * @return void HAL 回调的固定签名。
 */
void HAL_GPIO_EXTI_Callback(uint16_t u16_gpio_pin);

#endif /* KEY_USER_ISR_HANDLERS_H */
