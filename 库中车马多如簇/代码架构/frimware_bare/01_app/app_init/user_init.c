/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file user_init.c
 *
 * @par dependencies
 * - user_init.h
 * - ../app_message/msg_queue.h
 * - ../app_control/app_control.h
 * - ../app_loop/app_loop.h
 *
 * @author YA
 *
 * @brief 裸机 App 层统一初始化实现。
 *
 * Processing flow:
 *
 * main.c -> UserAppInit -> 队列/状态机/主循环等子模块 init -> while(1)。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "user_init.h"

#include "../app_message/msg_queue.h"
#include "../app_control/app_control.h"
#include "../app_loop/app_loop.h"

/**
 * @brief 裸机 App 层初始化入口，由 main 在进入 while(1) 前调用。
 *
 * Steps:
 *  1. 初始化事件队列 g_msg_queue。
 *  2. 初始化状态机当前状态。
 *  3. 挂载定时器 ISR 回调。
 *  4. 任一子模块 init 失败则立即返回错误码。
 *
 * @return app_status_t : 操作状态。
 *
 */
app_status_t UserAppInit(void)
{
    app_status_t status;

    status = queue_init(&g_msg_queue);
    if (APP_STATUS_OK != status)
    {
        return status;
    }

    status = app_control_init();
    if (APP_STATUS_OK != status)
    {
        return status;
    }

    status = app_loop_init();
    if (APP_STATUS_OK != status)
    {
        return status;
    }

    //----------------------------业务填充begin----------------------------//
    // 后续各层(driver/service/bsp 等) init 在此按顺序接入。
    //-----------------------------业务填充end-----------------------------//

    return APP_STATUS_OK;
}
