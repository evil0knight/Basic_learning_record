/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc. or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_task_reso_config.h
 * @par dependencies
 * - osal.h
 * - stdbool.h
 * - stddef.h
 * @author YA
 * @brief 定义按键任务及队列资源配置表。
 *
 * Processing flow:
 * 创建队列 -> 由 App 初始化代码遍历任务表创建任务。
 *
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef KEY_USER_TASK_RESO_CONFIG_H
#define KEY_USER_TASK_RESO_CONFIG_H

#include "osal.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum
{
    USER_TASK_IDX_THREAD_5MS = 0,
    USER_TASK_IDX_MAX
} en_user_task_idx_t;

typedef enum
{
    USER_QUEUE_IDX_SHUTDOWN_KEY = 0,
    USER_QUEUE_IDX_MAX
} en_user_queue_idx_t;

typedef struct
{
    bool b_enabled;
    const char *p_name;
    osal_task_entry_t pf_entry;
    size_t sz_stack_depth;
    osal_priority_t u32_priority;
    void *p_argument;
    osal_task_handle_t p_handle;
} st_user_task_config_t;

typedef struct
{
    size_t sz_depth;
    size_t sz_item_size;
    osal_queue_handle_t p_handle;
} st_user_queue_config_t;

extern st_user_task_config_t g_st_user_tasks[USER_TASK_IDX_MAX];
extern st_user_queue_config_t g_st_user_queues[USER_QUEUE_IDX_MAX];

/**
 * @brief 创建按键队列，不创建任务。
 * @param[in] 无；由 App 初始化阶段调用一次。
 * @return osal_status_t 队列创建状态。
 */
osal_status_t user_os_resources_init(void);

/**
 * @brief 获取按键队列，通过输出参数返回句柄。
 * @param[in] en_queue_idx 队列索引。
 * @param[out] p_queue 已创建的队列句柄。
 * @return osal_status_t 成功、无效参数或队列未创建错误。
 */
osal_status_t app_runtime_get_queue(en_user_queue_idx_t en_queue_idx,
                                   osal_queue_handle_t *p_queue);

#endif /* KEY_USER_TASK_RESO_CONFIG_H */
