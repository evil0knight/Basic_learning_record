/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file msg_queue.c
 *
 * @par dependencies
 * - msg_queue.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 表驱法裸机框架：msg_queue.c
 *
 * 处理流程：中断入队，主循环按消息表与状态表处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes*********************************//
#include "msg_queue.h"

#include <stddef.h>
//********************************Includes*********************************//

msg_queue_t g_msg_queue;

/**
 * @brief 初始化消息队列。
 *
 * @param[in,out] p_queue 队列指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t queue_init(msg_queue_t * const p_queue)
{
    if (NULL == p_queue) {
        return APP_STATUS_INVALID_PARAM;
    }
    p_queue->u8_head = 0U;
    p_queue->u8_tail = 0U;
    p_queue->u8_count = 0U;
    return APP_STATUS_OK;
}

/**
 * @brief 检查消息并写入队列。
 *
 * @param[in,out] p_queue 队列指针。
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t queue_push(
    msg_queue_t * const p_queue,
    const event_message_t * const p_msg)
{
    if ((NULL == p_queue) || (NULL == p_msg)) {
        return APP_STATUS_INVALID_PARAM;
    }
    if ((APP_MESSAGE_DATA_LEN < p_msg->u16_length) ||
        ((unsigned int)p_msg->source_id >= (unsigned int)MSG_SRC_MAX)) {
        return APP_STATUS_INVALID_PARAM;
    }
    if (MSG_QUEUE_SIZE <= p_queue->u8_count) {
        return APP_STATUS_QUEUE_FULL;
    }
    p_queue->a_buffer[p_queue->u8_head] = *p_msg;
    p_queue->u8_head =
        (uint8_t)((p_queue->u8_head + 1U) % MSG_QUEUE_SIZE);
    p_queue->u8_count++;
    return APP_STATUS_OK;
}

/**
 * @brief 取出消息并更新队列游标。
 *
 * @param[in,out] p_queue 队列指针。
 * @param[out] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t queue_pop(
    msg_queue_t * const p_queue,
    event_message_t * const p_msg)
{
    if ((NULL == p_queue) || (NULL == p_msg)) {
        return APP_STATUS_INVALID_PARAM;
    }
    if (0U == p_queue->u8_count) {
        return APP_STATUS_QUEUE_EMPTY;
    }
    *p_msg = p_queue->a_buffer[p_queue->u8_tail];
    p_queue->u8_tail =
        (uint8_t)((p_queue->u8_tail + 1U) % MSG_QUEUE_SIZE);
    p_queue->u8_count--;
    return APP_STATUS_OK;
}
