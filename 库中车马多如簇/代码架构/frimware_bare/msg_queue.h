/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file msg_queue.h
 *
 * @par dependencies
 * - app_message.h
 *
 * @author YA
 *
 * @brief 表驱法裸机框架：msg_queue.h
 *
 * 处理流程：中断入队，主循环按消息表与状态表处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H

//********************************Includes*********************************//
#include "app_message.h"
//********************************Includes*********************************//

#define MSG_QUEUE_SIZE 8U

typedef struct {
    event_message_t a_buffer[MSG_QUEUE_SIZE];
    uint8_t u8_head;
    uint8_t u8_tail;
    uint8_t u8_count;
} msg_queue_t;

extern msg_queue_t g_msg_queue;

/**
 * @brief 初始化消息队列。
 *
 * @param[in,out] p_queue 队列指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t queue_init(msg_queue_t * const p_queue);
/**
 * @brief 检查消息并写入队列。
 *
 * @param[in,out] p_queue 队列指针。
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t queue_push(
    msg_queue_t * const p_queue,
    const event_message_t * const p_msg);
/**
 * @brief 取出消息并更新队列游标。
 *
 * @param[in,out] p_queue 队列指针。
 * @param[out] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t queue_pop(
    msg_queue_t * const p_queue,
    event_message_t * const p_msg);

#endif
