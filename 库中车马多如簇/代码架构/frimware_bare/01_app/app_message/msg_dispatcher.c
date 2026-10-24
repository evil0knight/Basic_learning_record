/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file msg_dispatcher.c
 *
 * @par dependencies
 * - msg_dispatcher.h
 * - app_control.h
 * - msg_queue.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 主循环事件读取和分发实现。
 *
 * Processing flow:
 *
 * 从队列取出 app_event_t 并交给 app_control_event_map()。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "msg_dispatcher.h"

#include "../app_control/app_control.h"
#include "msg_queue.h"

#include <stddef.h>
/**
 * @brief 执行 msg_dispatcher_dispatch 操作。
 *
 * @param[in] event : 函数调用参数。
 * @return static app_status_t : 函数执行结果。
 *
 */

static app_status_t msg_dispatcher_dispatch(app_event_t event)
{
    if ((unsigned int)event >= (unsigned int)EVENT_MAX)
    {
        return APP_STATUS_INVALID_PARAM;
    }

    return app_control_event_map(event);
}

/**
 * @brief 读取并处理队列中的全部事件。
 * @return app_status_t 操作状态。
 */
app_status_t do_read_message(void)
{
    app_event_t event;
    app_status_t status;
    app_status_t result = APP_STATUS_OK;

    status = queue_pop(&g_msg_queue, &event);
    while (APP_STATUS_OK == status)
    {
        status = msg_dispatcher_dispatch(event);
        if ((APP_STATUS_OK == result) && (APP_STATUS_OK != status))
        {
            result = status;
        }
        status = queue_pop(&g_msg_queue, &event);
    }

    if (APP_STATUS_QUEUE_EMPTY != status)
    {
        return status;
    }
    return result;
}




