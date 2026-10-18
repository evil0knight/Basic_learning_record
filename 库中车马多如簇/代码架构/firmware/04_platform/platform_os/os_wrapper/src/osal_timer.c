/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_timer.c
 *
 * @par dependencies
 * - osal_internal_globaldefs.h
 * - 对应的 OSAL 内部接口头文件
 *
 * @author YA
 *
 * @brief OSAL 软件定时器接口实现。
 *
 * 本文件负责定时器记录创建和参数检查，并转发到 OS implementation 层。
 *
 * Processing flow:
 *
 * 参数检查 -> 调用 OS implementation 接口 -> 返回 OSAL 状态。
 *
 * @version V1.0 2026-10-17
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#include "osal_internal_globaldefs.h"
#include "osal_internal_heap.h"
#include "osal_internal_timer.h"

/**
 * @brief OSAL 接口 osal_timer_create。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_timer_create(const char *name, osal_tick_t period,
                                bool auto_reload, osal_timer_callback_t callback,
                                void *argument, osal_timer_handle_t *timer)
{
    osal_status_t status;
    osal_timer_record_t *record;

    OSAL_RETURN_IF_NULL(name);
    OSAL_RETURN_IF_FALSE(osal_name_is_valid(name), OSAL_ERR_NAME_TOO_LONG);
    OSAL_RETURN_IF_FALSE(period > 0U, OSAL_ERR_INVALID_ARGUMENT);
    OSAL_RETURN_IF_NULL(callback);
    OSAL_RETURN_IF_NULL(timer);
    *timer = NULL;

    record = os_heap_alloc_impl(sizeof(*record));
    if (record == NULL)
    {
        return OSAL_ERR_NO_MEMORY;
    }

    memset(record, 0, sizeof(*record));
    memcpy(record->name, name, strlen(name) + 1U);
    record->period = period;
    record->auto_reload = auto_reload;
    record->callback = callback;
    record->argument = argument;

    status = os_timer_create_impl(record);
    if (status != OSAL_SUCCESS)
    {
        (void)os_heap_free_impl(record);
        return status;
    }

    *timer = record;
    return OSAL_SUCCESS;
}

/**
 * @brief OSAL 接口 osal_timer_start。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_timer_start(osal_timer_handle_t timer, osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(timer);
    return os_timer_start_impl(timer, timeout);
}

/**
 * @brief OSAL 接口 osal_timer_stop。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_timer_stop(osal_timer_handle_t timer, osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(timer);
    return os_timer_stop_impl(timer, timeout);
}

/**
 * @brief OSAL 接口 osal_timer_reset。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_timer_reset(osal_timer_handle_t timer, osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(timer);
    return os_timer_reset_impl(timer, timeout);
}

/**
 * @brief OSAL 接口 osal_timer_change_period。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_timer_change_period(osal_timer_handle_t timer,
                                       osal_tick_t new_period,
                                       osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(timer);
    OSAL_RETURN_IF_FALSE(new_period > 0U, OSAL_ERR_INVALID_ARGUMENT);
    return os_timer_change_period_impl(timer, new_period, timeout);
}

/**
 * @brief OSAL 接口 osal_timer_delete。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_timer_delete(osal_timer_handle_t timer, osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(timer);
    return os_timer_delete_impl(timer, timeout);
}

/**
 * @brief OSAL 接口 osal_timer_get_period。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_tick_t osal_timer_get_period(osal_timer_handle_t timer)
{
    return (timer == NULL) ? 0U : os_timer_get_period_impl(timer);
}
