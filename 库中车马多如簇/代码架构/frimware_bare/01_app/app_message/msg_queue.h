/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file msg_queue.h
 *
 * @par dependencies
 * - ../app_control/app_control.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 固定容量事件队列接口。
 *
 * Processing flow:
 *
 * 中断回调写入事件，主循环取出事件。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H

#include "../app_control/app_control.h"
#include "../../03_config/project_config.h"
#include <stdint.h>

#define MSG_QUEUE_SIZE CFG_EVENT_QUEUE_DEPTH

typedef struct
{
    app_event_t a_buffer[MSG_QUEUE_SIZE];
    uint8_t u8_head;
    uint8_t u8_tail;
    uint8_t u8_count;
} msg_queue_t;

extern msg_queue_t g_msg_queue;
/**
 * @brief 执行 queue_init 操作。
 *
 * @param[in] p_queue : 函数调用参数。
 * @return app_status_t : 函数执行结果。
 *
 */

app_status_t queue_init(msg_queue_t * const p_queue);
/**
 * @brief 执行 queue_push 操作。
 *
 * @param[in] p_queue : 函数调用参数。
 * @param[in] event : 函数调用参数。
 * @return app_status_t : 函数执行结果。
 *
 */
app_status_t queue_push(msg_queue_t * const p_queue, app_event_t event);
/**
 * @brief 执行 queue_pop 操作。
 *
 * @param[in] p_queue : 函数调用参数。
 * @param[in] p_event : 函数调用参数。
 * @return app_status_t : 函数执行结果。
 *
 */
app_status_t queue_pop(
    msg_queue_t * const p_queue,
    app_event_t * const p_event);

#endif /* MSG_QUEUE_H */




