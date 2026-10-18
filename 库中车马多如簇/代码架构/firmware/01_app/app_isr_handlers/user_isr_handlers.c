/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
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
 * @brief App 层中断回调注册与分发实现。
 *
 * Processing flow:
 *
 * 注册回调 -> 平台 ISR 调用 user_isr_dispatch -> 执行对应回调。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#include "user_isr_handlers.h"

typedef struct
{
    user_isr_callback_t callback;
    void *argument;
} user_isr_slot_t;

static user_isr_slot_t g_user_isr_slots[USER_ISR_EVENT_MAX];

/**
 * @brief 注册一个 App 中断回调。
 *
 * @param[in] event : 中断事件类型。
 * @param[in] callback : 回调函数，可为 NULL。
 * @param[in] argument : 回调私有参数。
 * @return osal_status_t : 注册状态。
 */
osal_status_t user_isr_callback_register(user_isr_event_t event,
                                         user_isr_callback_t callback,
                                         void *argument)
{
    if (event >= USER_ISR_EVENT_MAX)
    {
        return OSAL_ERR_INVALID_ARGUMENT;
    }

    g_user_isr_slots[event].callback = callback;
    g_user_isr_slots[event].argument = argument;
    return OSAL_SUCCESS;
}

/**
 * @brief 分发一个平台中断事件。
 *
 * @param[in] event : 中断事件类型。
 * @param[in] context : 平台中断上下文。
 * @return void : 无返回值。
 */
void user_isr_dispatch(user_isr_event_t event, void *context)
{
    user_isr_callback_t callback;

    if (event >= USER_ISR_EVENT_MAX)
    {
        return;
    }

    callback = g_user_isr_slots[event].callback;
    if (NULL != callback)
    {
        callback(event, context, g_user_isr_slots[event].argument);
    }
}
