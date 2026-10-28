/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc. or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.c
 * @par dependencies
 * - user_isr_handlers.h
 * - user_task_reso_config.h
 * - main.h
 * - osal.h
 * @author YA
 * @brief 处理 PA0 按键中断并投递队列消息。
 *
 * Processing flow:
 * PA0 中断 -> 检查队列 -> 非阻塞发送一个字节。
 *
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#include "user_isr_handlers.h"

#include "user_task_reso_config.h"
#include "main.h"
#include "osal.h"

/**
 * @brief 向按键队列投递数值 1。
 * @param[in] 无。
 * @return osal_status_t 获取句柄或发送消息的状态。
 */
static osal_status_t user_pa0_exti_callback(void)
{
    uint8_t u8_data = 1U;
    osal_queue_handle_t p_queue;
    osal_status_t en_status;

    /*---------------------- 参数检查 begin ----------------------*/
    /* 无入参；检查按键队列是否可用。 */
    en_status = app_runtime_get_queue(USER_QUEUE_IDX_SHUTDOWN_KEY, &p_queue);
    if (OSAL_SUCCESS != en_status)
    {
        return en_status;
    }
    /*----------------------- 参数检查 end -----------------------*/

    return osal_queue_send(p_queue, &u8_data, 0U);
}

/**
 * @brief 处理 PA0 按键的外部中断。
 * @param[in] u16_gpio_pin HAL 传入的引脚掩码。
 * @return void HAL 回调的固定签名。
 */
void HAL_GPIO_EXTI_Callback(uint16_t u16_gpio_pin)
{
    /*---------------------- 参数检查 begin ----------------------*/
    if (GPIO_PIN_0 != u16_gpio_pin)
    {
        return;
    }
    /*----------------------- 参数检查 end -----------------------*/

    (void)user_pa0_exti_callback();
}
