/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file main.c
 *
 * @par dependencies
 * - 01_app/app_control/app_control.h
 * - 01_app/app_init/user_init.h
 * - 01_app/app_loop/app_loop.h
 * - 01_app/app_message/msg_dispatcher.h
 *
 * @author YA
 *
 * @brief 裸机工程入口和主循环。
 *
 * Processing flow:
 *
 * UserAppInit() 统一初始化 App 层子模块，主循环并列调用事件分发与后台轮询。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "01_app/app_control/app_control.h"
#include "01_app/app_init/user_init.h"
#include "01_app/app_loop/app_loop.h"
#include "01_app/app_message/msg_dispatcher.h"



/**
 * @brief 初始化裸机框架并运行主循环。
 * @return int 初始化失败返回非零值。
 */
int main(void)
{
    if (APP_STATUS_OK != UserAppInit()) {
        return 1;
    }

    while (1) {
        //-------------------事件驱动：处理消息队列 begin-------------------//
        (void)do_read_message();
        //--------------------事件驱动：处理消息队列 end--------------------//
        //-----------后台轮询：周期关键处理由定时器 ISR 抢占 begin-----------//
        (void)do_loop();
        //------------后台轮询：周期关键处理由定时器 ISR 抢占 end------------//
    }
}




