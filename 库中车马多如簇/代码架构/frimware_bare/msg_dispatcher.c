/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file msg_dispatcher.c
 *
 * @par dependencies
 * - msg_dispatcher.h
 * - app_state.h
 * - msg_queue.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 表驱法裸机框架：msg_dispatcher.c
 *
 * 处理流程：中断入队，主循环按消息表与状态表处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes*********************************//
#include "msg_dispatcher.h"
#include "app_state.h"
#include "msg_queue.h"

#include <stddef.h>
//********************************Includes*********************************//

/**
 * @brief 将串口命令转换为应用事件。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t parse_uart1_data(const event_message_t * const p_msg);
/**
 * @brief 将 CAN 故障报文转换为应用事件。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t parse_can1_data(const event_message_t * const p_msg);
/**
 * @brief 解析 ADC 大端采样并检查阈值。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t parse_adc_data(const event_message_t * const p_msg);

static const msg_handler_t g_apf_handler_table[MSG_SRC_MAX] = {
    [MSG_SRC_UART1] = parse_uart1_data,
    [MSG_SRC_CAN1_RX] = parse_can1_data,
    [MSG_SRC_ADC_READY] = parse_adc_data
};

/**
 * @brief 按来源查表分发消息。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t msg_dispatcher_dispatch(
    const event_message_t * const p_msg)
{
    if ((NULL == p_msg) ||
        ((unsigned int)p_msg->source_id >= (unsigned int)MSG_SRC_MAX) ||
        (APP_MESSAGE_DATA_LEN < p_msg->u16_length)) {
        return APP_STATUS_INVALID_PARAM;
    }
    if (NULL == g_apf_handler_table[p_msg->source_id]) {
        return APP_STATUS_INVALID_PARAM;
    }
    return g_apf_handler_table[p_msg->source_id](p_msg);
}

/**
 * @brief 排空消息队列，返回遇到的首个错误。
 *
 * @note 无输入参数。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t do_read_message(void)
{
    event_message_t msg;
    app_status_t status;
    app_status_t result = APP_STATUS_OK;

    status = queue_pop(&g_msg_queue, &msg);
    while (APP_STATUS_OK == status) {
        status = msg_dispatcher_dispatch(&msg);
        if ((APP_STATUS_OK == result) && (APP_STATUS_OK != status)) {
            result = status;
        }
        status = queue_pop(&g_msg_queue, &msg);
    }
    if (APP_STATUS_QUEUE_EMPTY != status) {
        return status;
    }
    return result;
}

/**
 * @brief 将串口命令转换为应用事件。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t parse_uart1_data(const event_message_t * const p_msg)
{
    if ((NULL == p_msg) || (0U == p_msg->u16_length)) {
        return APP_STATUS_INVALID_PARAM;
    }
    switch (p_msg->au8_data[0]) {
        case 0xA1U:
            return app_state_process_event(EVENT_START, p_msg);
        case 0xA2U:
            return app_state_process_event(EVENT_STOP, p_msg);
        default:
            return app_state_process_event(EVENT_FAULT, p_msg);
    }
}

/**
 * @brief 将 CAN 故障报文转换为应用事件。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t parse_can1_data(const event_message_t * const p_msg)
{
    if ((NULL == p_msg) || (0U == p_msg->u16_length)) {
        return APP_STATUS_INVALID_PARAM;
    }
    if (0xE0U == p_msg->au8_data[0]) {
        return app_state_process_event(EVENT_FAULT, p_msg);
    }
    return APP_STATUS_OK;
}

/**
 * @brief 解析 ADC 大端采样并检查阈值。
 *
 * @param[in] p_msg 消息指针。
 * @return app_status_t：成功或具体错误状态。
 */
static app_status_t parse_adc_data(const event_message_t * const p_msg)
{
    uint16_t u16_adc_value;

    if ((NULL == p_msg) || (2U > p_msg->u16_length)) {
        return APP_STATUS_INVALID_PARAM;
    }
    u16_adc_value = (uint16_t)(
        ((uint16_t)p_msg->au8_data[0] << 8U) | p_msg->au8_data[1]);
    if (3000U < u16_adc_value) {
        return app_state_process_event(EVENT_FAULT, p_msg);
    }
    return APP_STATUS_OK;
}
