/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.c
 *
 * @par dependencies
 * - user_isr_handlers.h
 *
 * @author YA
 *
 * @brief HAL 中断回调入口，直接转发到 Platform 事件处理接口。
 *
 * Processing flow:
 *
 * HAL 回调 -> Platform core_xxx 事件处理 -> Driver 或队列通知。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "user_isr_handlers.h"

/* 按实际外设包含 Platform 头文件，并实现 HAL 固定签名回调。
 * 回调中检查引脚或外设句柄后，直接调用对应 core_xxx 事件处理接口。
 * 本模板不预设具体 MCU、外设和事件处理函数。
 */
