/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_types.h
 *
 * @par dependencies
 * - stdbool.h
 * - stddef.h
 * - stdint.h
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

#ifndef OSAL_TYPES_H
#define OSAL_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t osal_tick_t;
typedef uint32_t osal_priority_t;

typedef void *osal_task_handle_t;
typedef void *osal_queue_handle_t;
typedef void *osal_mutex_handle_t;
typedef void *osal_sema_handle_t;
typedef void *osal_timer_handle_t;
typedef void *osal_event_handle_t;

typedef void (*osal_task_entry_t)(void *argument);
typedef void (*osal_timer_callback_t)(osal_timer_handle_t timer, void *argument);

#define OSAL_WAIT_FOREVER UINT32_MAX

#endif
