/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.h
 *
 * @par dependencies
 * - osal.h
 *
 * @author YA
 *
 * @brief App 层中断回调注册与分发接口。
 *
 * Processing flow:
 *
 * 平台 ISR -> user_isr_dispatch -> 已注册的 App 回调。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef USER_ISR_HANDLERS_H
#define USER_ISR_HANDLERS_H

#include "osal.h"

typedef enum
{
    USER_ISR_EVENT_PLATFORM = 0,
    USER_ISR_EVENT_MAX
} user_isr_event_t;

typedef void (*user_isr_callback_t)(user_isr_event_t event,
                                    void *context,
                                    void *argument);

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
                                         void *argument);

/**
 * @brief 分发一个平台中断事件。
 *
 * @param[in] event : 中断事件类型。
 * @param[in] context : 平台中断上下文。
 * @return void : 无返回值。
 */
void user_isr_dispatch(user_isr_event_t event, void *context);

#endif /* USER_ISR_HANDLERS_H */
