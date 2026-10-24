/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file msg_dispatcher.h
 *
 * @par dependencies
 * - ../app_control/app_control.h
 *
 * @author YA
 *
 * @brief 主循环事件读取接口。
 *
 * Processing flow:
 *
 * do_read_message() 读取队列并调用应用控制层。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef MSG_DISPATCHER_H
#define MSG_DISPATCHER_H

#include "../app_control/app_control.h"

app_status_t do_read_message(void);

#endif /* MSG_DISPATCHER_H */



