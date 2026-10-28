/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file app_control.h
 *
 * @par dependencies
 * - 无
 *
 * @author YA
 *
 * @brief 表驱动的应用状态控制接口。
 *
 * Processing flow:
 *
 * app_control_event_map() 查找状态转移表并执行 pf_action。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef APP_CONTROL_H
#define APP_CONTROL_H

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
    EVENT_KEY0_PRESS,
    EVENT_KEY0_RELEASE,
    EVENT_KEY0_SHORT_PRESS,
    EVENT_KEY0_LONG_PRESS,
    EVENT_MAX
} app_event_t;

typedef enum
{
    STATE_IDLE = 0,
    STATE_RUNNING,
    STATE_ERROR,
    STATE_MAX
} app_state_t;

typedef app_status_t (*state_action_t)(void);

typedef struct
{
    app_state_t current_state;
    app_event_t event;
    app_state_t next_state;
    state_action_t pf_action;
} transition_t;
/**
 * @brief 执行 app_control_init 操作。
 *
 * @return app_status_t : 函数执行结果。
 *
 */

app_status_t app_control_init(void);
/**
 * @brief 执行 app_control_event_map 操作。
 *
 * @param[in] event : 函数调用参数。
 * @return app_status_t : 函数执行结果。
 *
 */
app_status_t app_control_event_map(app_event_t event);
/**
 * @brief 执行 app_control_get_current 操作。
 *
 * @param[in] p_state : 函数调用参数。
 * @return app_status_t : 函数执行结果。
 *
 */
app_status_t app_control_get_current(app_state_t * const p_state);

#endif /* APP_CONTROL_H */



