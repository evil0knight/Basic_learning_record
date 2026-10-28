/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file user_init.h
 *
 * @par dependencies
 * - ../app_control/app_control.h
 *
 * @author YA
 *
 * @brief 裸机 App 层统一初始化接口。
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
#ifndef USER_INIT_H
#define USER_INIT_H

//********************************Includes*********************************//

#include "../app_control/app_control.h"

//********************************Includes*********************************//

/**
 * @brief 执行 UserAppInit 操作。
 *
 * Steps:
 *  1. 初始化事件队列。
 *  2. 初始化状态机。
 *  3. 挂载定时器 ISR 回调。
 *  4. 挂载按键接口、对象及设备描述。
 *
 * @return app_status_t : 函数执行结果。
 *
 */
app_status_t UserAppInit(void);

#endif /* USER_INIT_H */
