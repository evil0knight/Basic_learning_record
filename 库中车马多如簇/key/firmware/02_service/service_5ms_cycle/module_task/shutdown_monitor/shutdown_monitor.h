/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file shutdown_monitor.h
 * @par dependencies
 * - osal.h
 * @author YA
 * @brief 声明长按关机检测接口。
 *
 * Processing flow:
 * 中断入队 -> 5ms 检测低电平 -> 累计 200 次后关机。
 *
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef KEY_SHUTDOWN_MONITOR_H
#define KEY_SHUTDOWN_MONITOR_H

#include "osal.h"

/**
 * @brief 执行一次 5ms 按键检测。
 *
 * Steps:
 *  1. 收到队列消息后开始检测。
 *  2. 低电平累计 200 次后执行电源操作。
 *  3. 松开时清零，达到门限后持续按住则每轮执行电源操作。
 *
 * @param[in] 无。
 * @return osal_status_t 队列未创建返回指针错误，正常检测返回成功。
 * @note 成功只表示完成检测，电源接口没有返回值。
 */
osal_status_t user_shutdown_monitor_5ms(void);

#endif /* KEY_SHUTDOWN_MONITOR_H */
