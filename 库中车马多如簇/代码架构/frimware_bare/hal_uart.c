/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file hal_uart.c
 *
 * @par dependencies
 * - app_event_flag.h
 * - app_message.h
 * - msg_queue.h
 *
 * @author YA
 *
 * @brief 表驱法裸机框架：hal_uart.c
 *
 * 处理流程：中断入队，主循环按消息表与状态表处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes*********************************//
#include "app_event_flag.h"
#include "app_message.h"
#include "msg_queue.h"
//********************************Includes*********************************//

/* 平台实现：非阻塞读取、限制写入容量并清除中断源。 */
/**
 * @brief 由平台提供的非阻塞串口读取接口。
 *
 * @param[out] p_u8_buffer 接收缓冲区指针。
 * @param[in] u16_capacity 接收缓冲区容量。
 * @param[out] p_u16_length 实际接收长度输出指针。
 * @return app_status_t：成功或具体错误状态。
 */
extern app_status_t hal_uart_read(
    uint8_t * const p_u8_buffer, uint16_t u16_capacity,
    uint16_t * const p_u16_length);

/**
 * @brief 接收串口消息并投递到队列。
 *
 * @note 无输入参数。
 * @return 无；硬件中断入口遵循平台签名。
 */
void UART1_IRQHandler(void)
{
    event_message_t msg = {0};
    app_status_t status;

    msg.source_id = MSG_SRC_UART1;
    status = hal_uart_read(
        msg.au8_data, APP_MESSAGE_DATA_LEN, &msg.u16_length);
    if ((APP_STATUS_OK != status) || (0U == msg.u16_length) ||
        (APP_MESSAGE_DATA_LEN < msg.u16_length)) {
        return;
    }
    if (APP_STATUS_OK == queue_push(&g_msg_queue, &msg)) {
        (void)set_event_flag(APP_EVENT_MESSAGE_READY);
    }
}
