/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file app_event.h
 *
 * @par dependencies
 * - 无。
 *
 * @author YA
 *
 * @brief 定义应用状态、事件和处理状态码。
 *
 * Processing flow:
 *
 * Driver 产生事件，应用控制层按事件查表处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef APP_EVENT_H
#define APP_EVENT_H

typedef enum
{
    APP_STATUS_OK = 0,
    APP_STATUS_INVALID_PARAM,
    APP_STATUS_QUEUE_EMPTY,
    APP_STATUS_QUEUE_FULL,
    APP_STATUS_NO_TRANSITION,
    APP_STATUS_IO_ERROR
} app_status_t;

typedef enum
{
    EVENT_NONE = 0,
    EVENT_UART1_RX_READY,
    EVENT_CAN1_RX_READY,
    EVENT_ADC_READY,
    EVENT_START,
    EVENT_STOP,
    EVENT_FAULT,
    EVENT_MAX
} app_event_t;

#endif /* APP_EVENT_H */

