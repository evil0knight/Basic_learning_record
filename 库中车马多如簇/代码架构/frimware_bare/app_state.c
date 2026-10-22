/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file app_state.c
 *
 * @par dependencies
 * - app_state.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 表驱法裸机框架：app_state.c
 *
 * 处理流程：中断入队，主循环按消息表与状态表处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes*********************************//
#include "app_state.h"

#include <stddef.h>
//********************************Includes*********************************//

/**
 * @brief 记录启动命令。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t action_on_start(const event_message_t * const p_msg);
/**
 * @brief 清除启动命令记录。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t action_on_stop(const event_message_t * const p_msg);
/**
 * @brief 记录故障消息来源。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t action_on_fault(const event_message_t * const p_msg);

static app_state_t g_current_state = STATE_IDLE;
static volatile uint8_t g_u8_last_command = 0U;
static volatile msg_source_t g_last_fault_source = MSG_SRC_MAX;

static const transition_t g_transition_table[] = {
    { STATE_IDLE, EVENT_START, STATE_RUNNING, action_on_start },
    { STATE_RUNNING, EVENT_STOP, STATE_IDLE, action_on_stop },
    { STATE_RUNNING, EVENT_FAULT, STATE_ERROR, action_on_fault },
    { STATE_ERROR, EVENT_STOP, STATE_IDLE, action_on_stop }
};

/**
 * @brief 初始化应用状态。
 *
 * @note 无输入参数。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t app_state_init(void)
{
    g_current_state = STATE_IDLE;
    g_u8_last_command = 0U;
    g_last_fault_source = MSG_SRC_MAX;
    return APP_STATUS_OK;
}

/**
 * @brief 查表执行动作，成功后更新状态。
 *
 * @param[in] event 应用事件。
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t app_state_process_event(
    app_event_t event, const event_message_t * const p_msg)
{
    size_t u_index;
    const size_t u_table_size =
        sizeof(g_transition_table) / sizeof(g_transition_table[0]);
    app_status_t status;

    if ((NULL == p_msg) ||
        ((unsigned int)event >= (unsigned int)EVENT_MAX) ||
        ((unsigned int)p_msg->source_id >= (unsigned int)MSG_SRC_MAX) ||
        (APP_MESSAGE_DATA_LEN < p_msg->u16_length)) {
        return APP_STATUS_INVALID_PARAM;
    }
    for (u_index = 0U; u_index < u_table_size; u_index++) {
        if ((g_transition_table[u_index].current_state ==
             g_current_state) &&
            (g_transition_table[u_index].event == event)) {
            if (NULL == g_transition_table[u_index].pf_action) {
                return APP_STATUS_INVALID_PARAM;
            }
            status = g_transition_table[u_index].pf_action(p_msg);
            if (APP_STATUS_OK == status) {
                g_current_state = g_transition_table[u_index].next_state;
            }
            return status;
        }
    }
    return APP_STATUS_NO_TRANSITION;
}

/**
 * @brief 通过输出参数获取当前状态。
 *
 * @param[out] p_state 当前状态输出指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t app_state_get_current(app_state_t * const p_state)
{
    if (NULL == p_state) {
        return APP_STATUS_INVALID_PARAM;
    }
    *p_state = g_current_state;
    return APP_STATUS_OK;
}

/**
 * @brief 记录启动命令。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t action_on_start(const event_message_t * const p_msg)
{
    if ((NULL == p_msg) || (0U == p_msg->u16_length)) {
        return APP_STATUS_INVALID_PARAM;
    }
    g_u8_last_command = p_msg->au8_data[0];
    return APP_STATUS_OK;
}

/**
 * @brief 清除启动命令记录。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t action_on_stop(const event_message_t * const p_msg)
{
    if (NULL == p_msg) {
        return APP_STATUS_INVALID_PARAM;
    }
    g_u8_last_command = 0U;
    return APP_STATUS_OK;
}

/**
 * @brief 记录故障消息来源。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t action_on_fault(const event_message_t * const p_msg)
{
    if (NULL == p_msg) {
        return APP_STATUS_INVALID_PARAM;
    }
    g_last_fault_source = p_msg->source_id;
    return APP_STATUS_OK;
}
