/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file msg_dispatcher.h
 *
 * @par dependencies
 * - app_message.h
 *
 * @author YA
 *
 * @brief 表驱法裸机框架：msg_dispatcher.h
 *
 * 处理流程：中断入队，主循环按消息表与状态表处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef MSG_DISPATCHER_H
#define MSG_DISPATCHER_H

//********************************Includes*********************************//
#include "app_message.h"
//********************************Includes*********************************//

typedef app_status_t (*msg_handler_t)(
    const event_message_t * const p_msg);

/**
 * @brief 按来源查表分发消息。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t msg_dispatcher_dispatch(
    const event_message_t * const p_msg);
/**
 * @brief 排空消息队列，返回遇到的首个错误。
 *
 * @note 无输入参数。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t do_read_message(void);

#endif
