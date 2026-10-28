/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc. or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file service_5ms_cycle.c
 * @par dependencies
 * - service_5ms_cycle.h
 * - module_task/shutdown_monitor/shutdown_monitor.h
 * - osal.h
 * @author YA
 * @brief 执行周期按键检测和 5ms 延时。
 *
 * Processing flow:
 * 按键检测 -> 延时 5ms -> 下一轮。
 *
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#include "service_5ms_cycle.h"

#include "module_task/shutdown_monitor/shutdown_monitor.h"
#include "osal.h"

/**
 * @brief 循环执行按键检测，每轮结束后相对延时 5ms。
 * @param[in] p_argument 未使用，允许为空。
 * @return void OS 任务入口固定签名，不返回。
 */
void thread_5ms_task(void *p_argument)
{
    /*---------------------- 参数检查 begin ----------------------*/
    /* 任务参数未使用，允许为空。 */
    (void)p_argument;
    /*----------------------- 参数检查 end -----------------------*/

    while (1)
    {
        (void)user_shutdown_monitor_5ms();
        osal_task_delay(osal_ms_to_ticks(5U));
    }
}
