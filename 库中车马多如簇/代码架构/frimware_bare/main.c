/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file main.c
 *
 * @par dependencies
 * - 01_app/app_control/app_control.h
 * - 01_app/app_message/msg_dispatcher.h
 * - 01_app/app_message/msg_queue.h
 *
 * @author YA
 *
 * @brief 裸机工程入口和主循环。
 *
 * Processing flow:
 *
 * 初始化事件队列和应用控制层，主循环读取并处理应用事件。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "01_app/app_control/app_control.h"
#include "01_app/app_message/msg_dispatcher.h"
#include "01_app/app_message/msg_queue.h"



/**
 * @brief 初始化裸机框架并运行主循环。
 * @return int 初始化失败返回非零值。
 */
int main(void)
{
    
    if ((APP_STATUS_OK != queue_init(&g_msg_queue)) ||
        (APP_STATUS_OK != app_control_init())) {
        return 1;
    }
    
    while (1) {
        
        (void)do_read_message();
        
        

        
    }
}




