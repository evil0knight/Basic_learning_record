/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file app_state.h
 *
 * @par dependencies
 * - app_event.h
 * - app_message.h
 *
 * @author YA
 *
 * @brief 表驱法裸机框架：app_state.h
 *
 * 处理流程：中断入队，主循环按消息表与状态表处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef APP_STATE_H
#define APP_STATE_H

//********************************Includes*********************************//
#include "app_event.h"
#include "app_message.h"
//********************************Includes*********************************//

typedef enum {
    STATE_IDLE = 0,
    STATE_RUNNING,
    STATE_ERROR,
    STATE_MAX
} app_state_t;

typedef app_status_t (*state_action_t)(
    const event_message_t * const p_msg);

typedef struct {
    app_state_t current_state;
    app_event_t event;
    app_state_t next_state;
    state_action_t pf_action;
} transition_t;

/**
 * @brief 初始化应用状态。
 *
 * @note 无输入参数。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t app_state_init(void);
/**
 * @brief 查表执行动作，成功后更新状态。
 *
 * @param[in] event 应用事件。
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t app_state_process_event(
    app_event_t event, const event_message_t * const p_msg);
/**
 * @brief 通过输出参数获取当前状态。
 *
 * @param[out] p_state 当前状态输出指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t app_state_get_current(app_state_t * const p_state);

#endif
