/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file app_control.c
 *
 * @par dependencies
 * - app_control.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 应用状态转移表和事件处理实现。
 *
 * Processing flow:
 *
 * 事件查表后执行已接入的 Service 动作，并在成功时更新状态。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "app_control.h"

#include <stddef.h>

static app_state_t g_current_state = STATE_IDLE;

static const transition_t g_transition_table[] =
{
    { STATE_MAX,     EVENT_UART1_RX_READY, STATE_MAX, NULL },
    { STATE_MAX,     EVENT_CAN1_RX_READY,  STATE_MAX, NULL },
    { STATE_MAX,     EVENT_ADC_READY,      STATE_MAX, NULL },
    { STATE_IDLE,    EVENT_START,          STATE_RUNNING, NULL },
    { STATE_RUNNING, EVENT_STOP,           STATE_IDLE,    NULL },
    { STATE_RUNNING, EVENT_FAULT,          STATE_ERROR,   NULL },
    { STATE_ERROR,   EVENT_STOP,           STATE_IDLE,    NULL },
    { STATE_MAX,     EVENT_KEY0_PRESS,     STATE_MAX,     NULL },
    { STATE_MAX,     EVENT_KEY0_RELEASE,   STATE_MAX,     NULL },
    { STATE_MAX,     EVENT_KEY0_SHORT_PRESS, STATE_MAX,   NULL },
    { STATE_MAX,     EVENT_KEY0_LONG_PRESS, STATE_MAX,    NULL }
};
/**
 * @brief 初始化应用状态。
 * @return app_status_t 操作状态。
 */
app_status_t app_control_init(void)
{
    //-------------------------参数检查 begin-------------------------//
    /* 无入参，恢复初始应用状态。 */
    //--------------------------参数检查 end--------------------------//
    g_current_state = STATE_IDLE;
    return APP_STATUS_OK;
}

/**
 * @brief 按事件查找状态转移表并执行 pf_action。
 * @param[in] event 应用事件。
 * @return app_status_t 操作状态。
 */
app_status_t app_control_event_map(app_event_t event)
{
    size_t u_index;
    const size_t u_table_size =
        sizeof(g_transition_table) / sizeof(g_transition_table[0]);
    const transition_t *p_transition = NULL;
    app_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((unsigned int)event >= (unsigned int)EVENT_MAX)
    {
        return APP_STATUS_INVALID_PARAM;
    }

    //--------------------------参数检查 end--------------------------//

    for (u_index = 0U; u_index < u_table_size; u_index++)
    {
        if ((g_transition_table[u_index].event == event) &&
            ((g_transition_table[u_index].current_state == g_current_state) ||
             (g_transition_table[u_index].current_state == STATE_MAX)))
        {
            p_transition = &g_transition_table[u_index];
            break;
        }
    }

    if ((NULL == p_transition) || (NULL == p_transition->pf_action))
    {
        return APP_STATUS_NO_TRANSITION;
    }

    status = p_transition->pf_action();
    if ((APP_STATUS_OK == status) &&
        (STATE_MAX != p_transition->next_state))
    {
        g_current_state = p_transition->next_state;
    }
    return status;
}

/**
 * @brief 获取当前应用状态。
 * @param[out] p_state 状态输出指针。
 * @return app_status_t 操作状态。
 */
app_status_t app_control_get_current(app_state_t * const p_state)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_state)
    {
        return APP_STATUS_INVALID_PARAM;
    }

    //--------------------------参数检查 end--------------------------//

    *p_state = g_current_state;
    return APP_STATUS_OK;
}

