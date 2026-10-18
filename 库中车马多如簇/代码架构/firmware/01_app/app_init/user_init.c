/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_init.c
 *
 * @par dependencies
 * - user_init.h
 * - user_task_reso_config.h
 *
 * @author YA
 *
 * @brief App 层初始化任务实现。
 *
 * Processing flow:
 *
 * main.c -> app_periph_init -> UserAppTask_Init ->
 * userTaskInitFunction -> user_os_resources_init -> 任务配置表循环创建。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#include "user_init.h"

#ifdef DEBUG_User_Init
#include <stdio.h>
#endif

#include "user_task_reso_config.h"

#ifndef DEBUG_OUT
#ifdef DEBUG_User_Init
#define DEBUG_OUT(...) printf(__VA_ARGS__)
#else
#define DEBUG_OUT(...) ((void)0)
#endif
#endif

static osal_task_handle_t g_user_task_init_handle;

/**
 * @brief 检查初始化状态并在失败时停留在故障状态。
 *
 * Steps:
 *  1. 判断 OSAL 操作是否成功。
 *  2. 失败时在启用调试输出的情况下打印错误信息。
 *  3. 保持当前任务运行，等待目标工程复位或调试处理。
 *
 * @param[in] message : 错误描述。
 * @param[in] status : OSAL 错误码。
 * @return void : 成功时返回，失败时不会正常返回。
 */
static void user_init_check_status(osal_status_t status, const char *message)
{
    if (OSAL_SUCCESS == status)
    {
        return;
    }

#ifdef DEBUG_User_Init
    DEBUG_OUT("%s, error code: %d\n", message, status);
#else
    (void)message;
    (void)status;
#endif

    while (1)
    {
    }
}

/**
 * @brief App 初始化任务入口。
 *
 * Steps:
 *  1. 初始化 App 使用的 OSAL 资源。
 *  2. 创建已启用的 App 任务。
 *  3. 删除当前初始化任务。
 *
 * @param[in] argument : 未使用的任务参数。
 * @return void : 无返回值。
 */
static void userTaskInitFunction(void *argument)
{    osal_status_t status;
    size_t index;

    (void)argument;

    /* 初始化 OS 资源(user_task_reso_config里的,队列,信号量之类的初始化)*/
    status = user_os_resources_init();
    user_init_check_status(status, "OS resources init failed");

    /* 初始化各个任务。 */
    for (index = 0U; index < USER_TASK_IDX_MAX; ++index)
    {
        if (false == g_user_tasks[index].enabled)
        {
            continue;
        }

        if ((NULL == g_user_tasks[index].name) ||
            (NULL == g_user_tasks[index].entry))
        {
#ifdef DEBUG_User_Init
            DEBUG_OUT("%u task configuration invalid\n",
                      (unsigned int)index);
#endif
            continue;
        }

        status = osal_task_create(g_user_tasks[index].name,
                                  g_user_tasks[index].entry,
                                  g_user_tasks[index].stack_depth,
                                  g_user_tasks[index].priority,
                                  &g_user_tasks[index].handle,
                                  g_user_tasks[index].argument);
        user_init_check_status(status, g_user_tasks[index].name);
    }

    (void)osal_task_delete(NULL);
}

/**
 * @brief 创建 App 初始化任务。
 *
 * @param[in] None
 * @return void : 无返回值。
 */
void UserAppTask_Init(void)
{
    osal_status_t status;

    status = osal_task_create("userTask", userTaskInitFunction, 256U, 3U,
                              &g_user_task_init_handle, NULL);
    user_init_check_status(status, "userTask create failed");
}
