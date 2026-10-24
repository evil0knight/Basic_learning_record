/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_task_reso_config.c
 *
 * @par dependencies
 * - user_task_reso_config.h
 * - rtos_config.h
 *
 * @author YA
 *
 * @brief App 层任务与 OSAL 资源配置实现。
 *
 * Processing flow:
 *
 * 配置表 -> 资源创建 -> 任务创建；失败时按逆序释放已创建资源。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#include "user_task_reso_config.h"

#include "rtos_config.h"

#ifndef __WEAK
#if defined(__GNUC__)
#define __WEAK __attribute__((weak))
#else
#define __WEAK
#endif
#endif
/**
 * @brief 执行 thread_5ms_task 操作。
 *
 * @param[in] argument : 函数调用参数。
 * @return __WEAK void : 函数执行结果。
 *
 */

__WEAK void thread_5ms_task(void *argument);
/**
 * @brief 执行 sensor_polling_task 操作。
 *
 * @param[in] argument : 函数调用参数。
 * @return __WEAK void : 函数执行结果。
 *
 */
__WEAK void sensor_polling_task(void *argument);
/**
 * @brief 执行 display_refresh_task 操作。
 *
 * @param[in] argument : 函数调用参数。
 * @return __WEAK void : 函数执行结果。
 *
 */
__WEAK void display_refresh_task(void *argument);
/**
 * @brief 执行 low_power_monitor_task 操作。
 *
 * @param[in] argument : 函数调用参数。
 * @return __WEAK void : 函数执行结果。
 *
 */
__WEAK void low_power_monitor_task(void *argument);
/**
 * @brief 执行 server_watchdog_task 操作。
 *
 * @param[in] argument : 函数调用参数。
 * @return __WEAK void : 函数执行结果。
 *
 */
__WEAK void server_watchdog_task(void *argument);
/**
 * @brief 执行 adc_transfer_task 操作。
 *
 * @param[in] argument : 函数调用参数。
 * @return __WEAK void : 函数执行结果。
 *
 */
__WEAK void adc_transfer_task(void *argument);
/**
 * @brief 执行 adc_sample_task 操作。
 *
 * @param[in] argument : 函数调用参数。
 * @return __WEAK void : 函数执行结果。
 *
 */
__WEAK void adc_sample_task(void *argument);


user_task_config_t g_user_tasks[USER_TASK_IDX_MAX] =
{
    {false, "Thread_5ms_Task", thread_5ms_task, CFG_TASK_XXX_STACK_SIZE,
     CFG_TASK_XXX_PRIORITY, NULL, NULL}
};

user_queue_config_t g_user_queues[USER_QUEUE_IDX_MAX] =
{
    {false, 2U, sizeof(uint32_t), NULL},
    {false, 2U, sizeof(uint32_t), NULL},
    {false, 2U, sizeof(void *), NULL}
};

user_mutex_config_t g_user_mutexes[USER_MUTEX_IDX_MAX] =
{
    {false, NULL},
    {false, NULL},
    {false, NULL},
    {false, NULL}
};

user_sema_config_t g_user_semaphores[USER_SEMA_IDX_MAX] =
{
    {false, true, 1U, 0U, NULL},
    {false, true, 1U, 0U, NULL},
    {false, true, 1U, 0U, NULL}
};

user_event_config_t g_user_events[USER_EVENT_IDX_MAX] =
{
    {false, NULL}
};

/**
 * @brief 释放已创建的任务。
 *
 * @param[in] None
 * @return void : 无返回值。
 */
static void user_tasks_release(void)
{
    size_t index;

    for (index = 0U; index < USER_TASK_IDX_MAX; ++index)
    {
        if (NULL != g_user_tasks[index].handle)
        {
            (void)osal_task_delete(g_user_tasks[index].handle);
            g_user_tasks[index].handle = NULL;
        }
    }
}

/**
 * @brief 释放全部 OSAL 资源。
 *
 * @param[in] None
 * @return void : 无返回值。
 */
void user_os_resources_deinit(void)
{
    size_t index;

    user_tasks_release();

    for (index = 0U; index < USER_SEMA_IDX_MAX; ++index)
    {
        if (NULL != g_user_semaphores[index].handle)
        {
            (void)osal_sema_delete(g_user_semaphores[index].handle);
            g_user_semaphores[index].handle = NULL;
        }
    }

    for (index = 0U; index < USER_MUTEX_IDX_MAX; ++index)
    {
        if (NULL != g_user_mutexes[index].handle)
        {
            (void)osal_mutex_delete(g_user_mutexes[index].handle);
            g_user_mutexes[index].handle = NULL;
        }
    }

    for (index = 0U; index < USER_QUEUE_IDX_MAX; ++index)
    {
        if (NULL != g_user_queues[index].handle)
        {
            (void)osal_queue_delete(g_user_queues[index].handle);
            g_user_queues[index].handle = NULL;
        }
    }

    for (index = 0U; index < USER_EVENT_IDX_MAX; ++index)
    {
        if (NULL != g_user_events[index].handle)
        {
            (void)osal_event_group_delete(g_user_events[index].handle);
            g_user_events[index].handle = NULL;
        }
    }
}

/**
 * @brief 创建 App 配置表中的 OSAL 资源。
 *
 * Steps:
 *  1. 创建启用的队列。
 *  2. 创建启用的互斥量。
 *  3. 创建启用的信号量。
 *
 * @param[in] None
 * @return osal_status_t : 创建状态。
 */
osal_status_t user_os_resources_init(void)
{
    osal_status_t status;                  /* 保存当前结果。 */
    size_t index;                          /* 遍历资源配置表的索引。 */

    /* 创建配置表的队列。 */
    for (index = 0U; index < USER_QUEUE_IDX_MAX; ++index)
    {
        /* 未启用的队列不申请内核资源。 */
        if (false == g_user_queues[index].enabled)
        {
            continue;
        }

        /* 按配置创建队列并保存句柄。 */
        status = osal_queue_create(g_user_queues[index].depth,
                                   g_user_queues[index].item_size,
                                   &g_user_queues[index].handle);
        if (OSAL_SUCCESS != status)
        {
            /* 创建失败时释放此前已经创建的全部资源。 */
            user_os_resources_deinit();
            return status;
        }
    }

    /* 创建配置表的互斥量。 */
    for (index = 0U; index < USER_MUTEX_IDX_MAX; ++index)
    {
        /* 未启用的互斥量不申请内核资源。 */
        if (false == g_user_mutexes[index].enabled)
        {
            continue;
        }

        /* 创建互斥量并保存句柄。 */
        status = osal_mutex_create(&g_user_mutexes[index].handle);
        if (OSAL_SUCCESS != status)
        {
            /* 创建失败时释放队列、互斥量等已创建资源。 */
            user_os_resources_deinit();
            return status;
        }
    }

    /* 创建配置表的二值信号量。 */
    for (index = 0U; index < USER_SEMA_IDX_MAX; ++index)
    {
        /* 未启用的信号量不申请内核资源。 */
        if (false == g_user_semaphores[index].enabled)
        {
            continue;
        }

        /* 创建信号量并保存句柄。 */
        status = osal_sema_binary_create(&g_user_semaphores[index].handle);
        if (OSAL_SUCCESS != status)
        {
            /* 创建失败时释放此前已经创建的全部资源。 */
            user_os_resources_deinit();
            return status;
        }
    }

    /* 创建配置表中的事件组。 */
    for (index = 0U; index < USER_EVENT_IDX_MAX; ++index)
    {
        if (false == g_user_events[index].enabled)
        {
            continue;
        }

        status = osal_event_group_create(&g_user_events[index].handle);
        if (OSAL_SUCCESS != status)
        {
            user_os_resources_deinit();
            return status;
        }
    }

    /* 所有启用的 OSAL 资源均创建成功。 */
    return OSAL_SUCCESS;
}

/**
 * @brief 获取指定任务句柄。
 *
 * @param[in] index : 任务索引。
 * @return osal_task_handle_t : 任务句柄，无效索引返回 NULL。
 */
osal_task_handle_t user_task_get(user_task_idx_t index)
{
    return (index < USER_TASK_IDX_MAX) ? g_user_tasks[index].handle : NULL;
}

/**
 * @brief 获取指定队列句柄。
 *
 * @param[in] index : 队列索引。
 * @return osal_queue_handle_t : 队列句柄，无效索引返回 NULL。
 */
osal_queue_handle_t user_queue_get(user_queue_idx_t index)
{
    return (index < USER_QUEUE_IDX_MAX) ? g_user_queues[index].handle : NULL;
}

/**
 * @brief 获取指定互斥量句柄。
 *
 * @param[in] index : 互斥量索引。
 * @return osal_mutex_handle_t : 互斥量句柄，无效索引返回 NULL。
 */
osal_mutex_handle_t user_mutex_get(user_mutex_idx_t index)
{
    return (index < USER_MUTEX_IDX_MAX) ? g_user_mutexes[index].handle : NULL;
}

/**
 * @brief 获取指定信号量句柄。
 *
 * @param[in] index : 信号量索引。
 * @return osal_sema_handle_t : 信号量句柄，无效索引返回 NULL。
 */
osal_sema_handle_t user_sema_get(user_sema_idx_t index)
{
    return (index < USER_SEMA_IDX_MAX)
               ? g_user_semaphores[index].handle
               : NULL;
}

/**
 * @brief 获取指定事件组句柄。
 *
 * @param[in] index : 事件组索引。
 * @return osal_event_handle_t : 事件组句柄，无效索引返回 NULL。
 */
osal_event_handle_t user_event_get(user_event_idx_t index)
{
    return (index < USER_EVENT_IDX_MAX) ? g_user_events[index].handle : NULL;
}



//预定义task写这里
/**
 * @brief 执行 thread_5ms_task 操作。
 *
 * @param[in] argument : 函数调用参数。
 * @return __WEAK void : 函数执行结果。
 *
 */
__WEAK void thread_5ms_task(void *argument)
{
    (void)argument;
    while (1)
    {
    }
}




