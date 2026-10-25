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
 * @brief 固定容量事件队列实现。
 *
 * Processing flow:
 *
 * 事件按先进先出顺序写入和取出。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "msg_queue.h"
#include "../../04_platform/platform_mcu/critical/inc/critical_port.h"

#include <stddef.h>

msg_queue_t g_msg_queue;

/**
 * @brief 初始化事件队列。
 * @param[in,out] p_queue 队列指针。
 * @return app_status_t 操作状态。
 */
app_status_t queue_init(msg_queue_t * const p_queue)
{
    if (NULL == p_queue)
    {
        return APP_STATUS_INVALID_PARAM;
    }

    p_queue->u8_head = 0U;
    p_queue->u8_tail = 0U;
    p_queue->u8_count = 0U;
    return APP_STATUS_OK;
}

/**
 * @brief 写入一个应用事件。
 * @param[in,out] p_queue 队列指针。
 * @param[in] event 应用事件。
 * @return app_status_t 操作状态。
 */
app_status_t queue_push(msg_queue_t * const p_queue, app_event_t event)
{
    uint32_t u32_irq_state;
    app_status_t en_status = APP_STATUS_OK;

    //----------------------------参数校验begin----------------------------//
    if ((NULL == p_queue) ||
        ((unsigned int)event >= (unsigned int)EVENT_MAX))
    {
        return APP_STATUS_INVALID_PARAM;
    }
    //-----------------------------参数校验end-----------------------------//

    // 保存中断状态，保护满队列检查、事件写入和游标更新。
    if (PLATFORM_CRITICAL_OK != platform_critical_enter(&u32_irq_state))
    {
        return APP_STATUS_IO_ERROR;
    }
    if ((uint8_t)MSG_QUEUE_SIZE <= p_queue->u8_count)
    {
        en_status = APP_STATUS_QUEUE_FULL;
    }
    else
    {
        p_queue->a_buffer[p_queue->u8_head] = event;
        p_queue->u8_head =
            (uint8_t)((p_queue->u8_head + 1U) % MSG_QUEUE_SIZE);
        p_queue->u8_count++;
    }
    // 满队列也从这里退出，恢复调用前的中断状态。
    if (PLATFORM_CRITICAL_OK != platform_critical_exit(u32_irq_state))
    {
        return APP_STATUS_IO_ERROR;
    }
    return en_status;
}

/**
 * @brief 取出一个应用事件。
 * @param[in,out] p_queue 队列指针。
 * @param[out] p_event 事件输出指针。
 * @return app_status_t 操作状态。
 */
app_status_t queue_pop(
    msg_queue_t * const p_queue,
    app_event_t * const p_event)
{
    uint32_t u32_irq_state;
    app_status_t en_status = APP_STATUS_OK;

    //----------------------------参数校验begin----------------------------//
    if ((NULL == p_queue) || (NULL == p_event))
    {
        return APP_STATUS_INVALID_PARAM;
    }
    //-----------------------------参数校验end-----------------------------//

    // 保护空队列检查、事件读取和游标更新，避免中断入队交错。
    if (PLATFORM_CRITICAL_OK != platform_critical_enter(&u32_irq_state))
    {
        return APP_STATUS_IO_ERROR;
    }
    if (0U == p_queue->u8_count)
    {
        en_status = APP_STATUS_QUEUE_EMPTY;
    }
    else
    {
        *p_event = p_queue->a_buffer[p_queue->u8_tail];
        p_queue->u8_tail =
            (uint8_t)((p_queue->u8_tail + 1U) % MSG_QUEUE_SIZE);
        p_queue->u8_count--;
    }
    // 恢复原状态后，调用者才能继续查表和解码。
    if (PLATFORM_CRITICAL_OK != platform_critical_exit(u32_irq_state))
    {
        return APP_STATUS_IO_ERROR;
    }
    return en_status;
}






