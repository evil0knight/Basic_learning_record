/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file os_freertos.h
 *
 * @par dependencies
 * - FreeRTOS.h
 * - event_groups.h
 * - queue.h
 * - semphr.h
 * - task.h
 * - timers.h
 * - osal_config.h
 * - osal_types.h
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

#ifndef OS_FREERTOS_H
#define OS_FREERTOS_H

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "osal_config.h"
#include "osal_types.h"

#if (OSAL_RTOS != OSAL_FREERTOS)
#error "This implementation requires OSAL_RTOS == OSAL_FREERTOS"
#endif

#define OSAL_PORT_IS_IN_ISR() (xPortIsInsideInterrupt() != pdFALSE)
/**
 * @brief 执行 osal_port_ticks 操作。
 *
 * @param[in] ticks : 函数调用参数。
 * @return static inline TickType_t : 函数执行结果。
 *
 */

static inline TickType_t osal_port_ticks(osal_tick_t ticks)
{
    return (ticks == OSAL_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)ticks;
}

#endif
