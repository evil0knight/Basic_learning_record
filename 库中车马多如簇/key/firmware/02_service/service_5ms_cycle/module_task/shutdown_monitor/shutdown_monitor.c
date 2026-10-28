/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file shutdown_monitor.c
 * @par dependencies
 * - shutdown_monitor.h
 * - hp4570_port.h
 * - gpio_port.h
 * - main.h
 * - user_task_reso_config.h
 * @author YA
 * @brief 检测低电平长按并执行关机操作。
 *
 * Processing flow:
 * 非阻塞取消息 -> 启动计数 -> 低电平累计 -> 关机或松开清零。
 *
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#include "shutdown_monitor.h"

#include "hp4570_port.h"
#include "gpio_port.h"
#include "main.h"
#include "user_task_reso_config.h"

#include <stddef.h>
#include <stdint.h>

static uint8_t g_u8_shutdown_key_event = 0U;

/**
 * @brief 检测按键、累计低电平次数并执行关机操作。
 * @param[in] 无。
 * @return osal_status_t 队列不可用时返回错误，否则返回成功。
 */
osal_status_t user_shutdown_monitor_5ms(void)
{
    static uint8_t u8_is_count_started = 0U;
    static uint16_t u16_key_hold_count = 0U;
    osal_queue_handle_t p_shutdown_key_queue;
    osal_status_t en_status;

    /*---------------------- 参数检查 begin ----------------------*/
    /* 无入参；检查按键队列是否可用。 */
    en_status = app_runtime_get_queue(USER_QUEUE_IDX_SHUTDOWN_KEY,
                                     &p_shutdown_key_queue);
    if (OSAL_SUCCESS != en_status)
    {
        return en_status;
    }
    /*----------------------- 参数检查 end -----------------------*/

    if (OSAL_SUCCESS == osal_queue_receive(p_shutdown_key_queue,
                                          &g_u8_shutdown_key_event, 0U))
    {
        u8_is_count_started = 1U;
    }
    else if (0U == u8_is_count_started)
    {
        return OSAL_SUCCESS;
    }

    if (GPIO_PIN_RESET == core_gpio_read_pin_direct(
            PA0_USER_KEY3_GPIO_Port, PA0_USER_KEY3_Pin))
    {
        u16_key_hold_count++;
        if (u16_key_hold_count >= 200U)
        {
            hp4570_set_powerkey_wk_en(1);
            hp4570_set_battery_fet_dis(1);
            u16_key_hold_count = 200U;
        }
    }
    else
    {
        u8_is_count_started = 0U;
        u16_key_hold_count = 0U;
    }

    return OSAL_SUCCESS;
}
