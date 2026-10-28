/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file user_init.c
 *
 * @par dependencies
 * - user_init.h
 * - msg_queue.h：目标工程的 App 事件队列
 * - ../app_control/app_control.h
 * - ../app_loop/app_loop.h
 * - bsp_adapter_port_key.h
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

#include "msg_queue.h"
#include "../app_control/app_control.h"
#include "../app_loop/app_loop.h"
#include "bsp_adapter_port_key.h"
#include <stddef.h>

/**
 * @brief 裸机 App 层初始化入口，由 main 在进入 while(1) 前调用。
 *
 * Steps:
 *  1. 初始化事件队列 g_msg_queue。
 *  2. 初始化状态机当前状态。
 *  3. 挂载定时器 ISR 回调。
 *  4. 挂载按键接口和对象，失败时立即返回错误码。
 *
 * @return app_status_t : 操作状态。
 *
 */
app_status_t UserAppInit(void)
{
    app_status_t status;

    //-------------------------参数检查 begin-------------------------//
    /* 无入参；仅在硬件和毫秒时基就绪后启动初始化。 */
    //--------------------------参数检查 end--------------------------//
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

    if (BSP_KEY_OK != drv_adapter_key_register())
    {
        return APP_STATUS_IO_ERROR;
    }

    return APP_STATUS_OK;
}
