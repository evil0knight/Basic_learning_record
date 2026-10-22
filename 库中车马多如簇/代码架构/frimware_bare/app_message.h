/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file app_message.h
 *
 * @par dependencies
 * - app_event.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 表驱法裸机框架：app_message.h
 *
 * 处理流程：中断入队，主循环按消息表与状态表处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef APP_MESSAGE_H
#define APP_MESSAGE_H

//********************************Includes*********************************//
#include "app_event.h"

#include <stdint.h>
//********************************Includes*********************************//

#define APP_MESSAGE_DATA_LEN 64U

typedef enum {
    MSG_SRC_UART1 = 0,
    MSG_SRC_CAN1_RX,
    MSG_SRC_ADC_READY,
    MSG_SRC_MAX
} msg_source_t;

typedef struct {
    msg_source_t source_id;
    uint16_t    u16_length;
    uint8_t     au8_data[APP_MESSAGE_DATA_LEN];
} event_message_t;

#endif
