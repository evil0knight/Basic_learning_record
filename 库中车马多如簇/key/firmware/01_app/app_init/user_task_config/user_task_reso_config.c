/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc. or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_task_reso_config.c
 * @par dependencies
 * - user_task_reso_config.h
 * - service_5ms_cycle.h
 * - rtos_config.h
 * @author YA
 * @brief 配置按键队列、5ms 任务和队列创建流程。
 *
 * Processing flow:
 * 资源表 -> 创建队列 -> App 使用任务表创建任务 -> ISR 与任务访问队列。
 *
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#include "user_task_reso_config.h"

#include "service_5ms_cycle.h"
#include "rtos_config.h"

#include <stdint.h>

st_user_task_config_t g_st_user_tasks[USER_TASK_IDX_MAX] =
{
    {true, "Thread_5ms_Task", thread_5ms_task,
     CFG_TASK_5MS_STACK_SIZE, CFG_TASK_5MS_PRIORITY, NULL, NULL}
};

st_user_queue_config_t g_st_user_queues[USER_QUEUE_IDX_MAX] =
{
    {1U, sizeof(uint8_t), NULL}
};

/**
 * @brief 创建深度为 1、元素为 uint8_t 的按键队列。
 * @param[in] 无；仅在启动阶段调用一次。
 * @return osal_status_t 参数或队列创建状态。
 */
osal_status_t user_os_resources_init(void)
{
    size_t sz_index;
    osal_status_t en_status;

    /* 目标工程应在任务和按键中断开始使用队列前调用。 */
    for (sz_index = 0U; sz_index < USER_QUEUE_IDX_MAX; sz_index++)
    {
        /*---------------------- 参数检查 begin ----------------------*/
        /* 无入参；检查队列尚未创建。 */
        if (NULL != g_st_user_queues[sz_index].p_handle)
        {
            return OSAL_ERR_INVALID_ARGUMENT;
        }
        /*----------------------- 参数检查 end -----------------------*/

        en_status = osal_queue_create(
            g_st_user_queues[sz_index].sz_depth,
            g_st_user_queues[sz_index].sz_item_size,
            &g_st_user_queues[sz_index].p_handle);
        if (OSAL_SUCCESS != en_status)
        {
            return en_status;
        }
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 检查索引与输出参数，返回已创建的队列。
 * @param[in] en_queue_idx 队列索引。
 * @param[out] p_queue 队列句柄输出地址。
 * @return osal_status_t 获取结果，失败时有效输出地址被置空。
 */
osal_status_t app_runtime_get_queue(en_user_queue_idx_t en_queue_idx,
                                   osal_queue_handle_t *p_queue)
{
    /*---------------------- 参数检查 begin ----------------------*/
    if (NULL == p_queue)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    *p_queue = NULL;
    if ((en_queue_idx < USER_QUEUE_IDX_SHUTDOWN_KEY) ||
        (en_queue_idx >= USER_QUEUE_IDX_MAX))
    {
        return OSAL_ERR_INVALID_ARGUMENT;
    }
    /*----------------------- 参数检查 end -----------------------*/

    *p_queue = g_st_user_queues[en_queue_idx].p_handle;
    if (NULL == *p_queue)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    return OSAL_SUCCESS;
}
