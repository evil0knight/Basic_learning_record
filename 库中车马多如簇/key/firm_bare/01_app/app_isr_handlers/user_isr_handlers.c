/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.c
 *
 * @par dependencies
 * - user_isr_handlers.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief HAL 中断回调指针定义。
 *
 * Processing flow:
 *
 * Handler 在初始化时将 Driver 回调写入对应指针，HAL 回调通过指针调用 Driver。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "user_isr_handlers.h"
#include "bsp_wrapper_key.h"

#include <stddef.h>

user_isr_callback_t pf_pin_interrupt_callback = NULL;
user_isr_callback_t pf_dma_interrupt_callback = NULL;
user_isr_callback_t pf_int_interrupt_callback = NULL;
user_isr_callback_t pf_tim_interrupt_callback = NULL;

/**
 * @brief 转发 EXTI 按键中断，其他设备分支按目标工程合并。
 * @param[in] u16_gpio_pin 单一 EXTI 引脚掩码。
 * @return void HAL 固定签名。
 */
void HAL_GPIO_EXTI_Callback(uint16_t u16_gpio_pin)
{
    //-------------------------参数检查 begin-------------------------//
    if ((0U == u16_gpio_pin) ||
        (0U != (u16_gpio_pin & (u16_gpio_pin - 1U))))
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)bsp_wrapper_key_irq_notify(u16_gpio_pin);
}
