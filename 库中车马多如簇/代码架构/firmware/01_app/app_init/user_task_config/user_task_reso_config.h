/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_task_reso_config.h
 *
 * @par dependencies
 * - osal.h
 * - stdbool.h
 * - stddef.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief App 层任务与 OSAL 资源配置接口。
 *
 * Processing flow:
 *
 * 配置表 -> user_os_resources_init -> 任务配置表循环创建。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef USER_TASK_RESO_CONFIG_H
#define USER_TASK_RESO_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "osal.h"

typedef enum
{
    USER_TASK_IDX_THREAD_5MS = 0,
    USER_TASK_IDX_SENSOR,
    USER_TASK_IDX_DISPLAY,
    USER_TASK_IDX_LOW_POWER,
    USER_TASK_IDX_WATCHDOG,
    USER_TASK_IDX_ADC_TRANSFER,
    USER_TASK_IDX_ADC_SAMPLE,
    USER_TASK_IDX_MAX
} user_task_idx_t;

typedef enum
{
    USER_QUEUE_IDX_SENSOR_DATA = 0,
    USER_QUEUE_IDX_UI_EVENT,
    USER_QUEUE_IDX_OTA_APP_DATA,
    USER_QUEUE_IDX_MAX
} user_queue_idx_t;

typedef enum
{
    USER_MUTEX_IDX_SENSOR = 0,
    USER_MUTEX_IDX_UI,
    USER_MUTEX_IDX_ADC,
    USER_MUTEX_IDX_OTA_EXT_FLASH,
    USER_MUTEX_IDX_MAX
} user_mutex_idx_t;

typedef enum
{
    USER_SEMA_IDX_SHUTDOWN = 0,
    USER_SEMA_IDX_WAKEUP,
    USER_SEMA_IDX_EXT_FLASH,
    USER_SEMA_IDX_MAX
} user_sema_idx_t;

typedef enum
{
    USER_EVENT_IDX_APP = 0,
    USER_EVENT_IDX_MAX
} user_event_idx_t;

typedef struct
{
    bool enabled;
    const char *name;
    osal_task_entry_t entry;
    size_t stack_depth;
    osal_priority_t priority;
    void *argument;
    osal_task_handle_t handle;
} user_task_config_t;

typedef struct
{
    bool enabled;
    size_t depth;
    size_t item_size;
    osal_queue_handle_t handle;
} user_queue_config_t;

typedef struct
{
    bool enabled;
    osal_mutex_handle_t handle;
} user_mutex_config_t;

typedef struct
{
    bool enabled;
    bool binary;
    uint32_t maximum;
    uint32_t initial;
    osal_sema_handle_t handle;
} user_sema_config_t;

typedef struct
{
    bool enabled;
    osal_event_handle_t handle;
} user_event_config_t;

extern user_task_config_t g_user_tasks[USER_TASK_IDX_MAX];
extern user_queue_config_t g_user_queues[USER_QUEUE_IDX_MAX];
extern user_mutex_config_t g_user_mutexes[USER_MUTEX_IDX_MAX];
extern user_sema_config_t g_user_semaphores[USER_SEMA_IDX_MAX];
extern user_event_config_t g_user_events[USER_EVENT_IDX_MAX];
/**
 * @brief 执行 user_os_resources_init 操作。
 *
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t user_os_resources_init(void);
/**
 * @brief 执行 user_os_resources_deinit 操作。
 *
 * @return void : 无返回值。
 *
 */
void user_os_resources_deinit(void);
/**
 * @brief 执行 user_task_get 操作。
 *
 * @param[in] index : 函数调用参数。
 * @return osal_task_handle_t : 函数执行结果。
 *
 */

osal_task_handle_t user_task_get(user_task_idx_t index);
/**
 * @brief 执行 user_queue_get 操作。
 *
 * @param[in] index : 函数调用参数。
 * @return osal_queue_handle_t : 函数执行结果。
 *
 */
osal_queue_handle_t user_queue_get(user_queue_idx_t index);
/**
 * @brief 执行 user_mutex_get 操作。
 *
 * @param[in] index : 函数调用参数。
 * @return osal_mutex_handle_t : 函数执行结果。
 *
 */
osal_mutex_handle_t user_mutex_get(user_mutex_idx_t index);
/**
 * @brief 执行 user_sema_get 操作。
 *
 * @param[in] index : 函数调用参数。
 * @return osal_sema_handle_t : 函数执行结果。
 *
 */
osal_sema_handle_t user_sema_get(user_sema_idx_t index);
/**
 * @brief 执行 user_event_get 操作。
 *
 * @param[in] index : 函数调用参数。
 * @return osal_event_handle_t : 函数执行结果。
 *
 */
osal_event_handle_t user_event_get(user_event_idx_t index);

#endif /* USER_TASK_RESO_CONFIG_H */
