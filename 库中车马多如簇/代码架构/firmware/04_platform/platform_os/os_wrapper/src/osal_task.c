/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_task.c
 *
 * @par dependencies
 * - osal_internal_globaldefs.h
 * - 对应的 OSAL 内部接口头文件
 *
 * @author YA
 *
 * @brief OSAL 任务接口实现。
 *
 * 本文件负责任务参数检查、任务记录组装，并转发到 OS implementation 层。
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
#include "osal_internal_task.h"

/**
 * @brief OSAL 接口 osal_task_create。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_task_create(const char *name, osal_task_entry_t entry,
                               size_t stack_depth, osal_priority_t priority,
                               osal_task_handle_t *task, void *argument)
{
    osal_task_record_t record;

    OSAL_RETURN_IF_NULL(name);
    OSAL_RETURN_IF_FALSE(osal_name_is_valid(name), OSAL_ERR_NAME_TOO_LONG);
    OSAL_RETURN_IF_NULL(entry);
    OSAL_RETURN_IF_NULL(task);
    OSAL_RETURN_IF_FALSE(stack_depth > 0U, OSAL_ERR_INVALID_SIZE);
    *task = NULL;

    memset(&record, 0, sizeof(record));
    memcpy(record.name, name, strlen(name) + 1U);
    record.entry = entry;
    record.stack_depth = stack_depth;
    record.priority = priority;
    record.argument = argument;
    return os_task_create_impl(&record, task);
}

/**
 * @brief OSAL 接口 osal_task_delete。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_task_delete(osal_task_handle_t task)
{
    return os_task_delete_impl(task);
}

/**
 * @brief OSAL 接口 osal_task_start_scheduler。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_task_start_scheduler(void)
{
    return os_task_start_scheduler_impl();
}

/**
 * @brief OSAL 接口 osal_task_suspend。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_task_suspend(osal_task_handle_t task)
{
    return os_task_suspend_impl(task);
}

/**
 * @brief OSAL 接口 osal_task_resume。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_task_resume(osal_task_handle_t task)
{
    OSAL_RETURN_IF_NULL(task);
    return os_task_resume_impl(task);
}

/**
 * @brief 延迟当前任务。
 * @param[in] ticks : 延迟的 OSAL tick 数。
 * @return void : 无返回值。
 */
void osal_task_delay(osal_tick_t ticks)
{
    os_task_delay_impl(ticks);
}

/**
 * @brief OSAL 接口 osal_task_get_tick_count。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_tick_t osal_task_get_tick_count(void)
{
    return os_task_get_tick_count_impl();
}

/**
 * @brief OSAL 接口 osal_task_get_current。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_task_handle_t osal_task_get_current(void)
{
    return os_task_get_current_impl();
}

/**
 * @brief OSAL 接口 osal_task_notify。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_task_notify(osal_task_handle_t task, uint32_t bits)
{
    OSAL_RETURN_IF_NULL(task);
    return os_task_notify_impl(task, bits);
}

/**
 * @brief OSAL 接口 osal_task_notify_from_isr。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_task_notify_from_isr(osal_task_handle_t task,
                                        uint32_t bits)
{
    OSAL_RETURN_IF_NULL(task);
    return os_task_notify_from_isr_impl(task, bits);
}

/**
 * @brief OSAL 接口 osal_task_notify_and_query。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_task_notify_and_query(osal_task_handle_t task,
                                         uint32_t bits, uint32_t *value)
{
    OSAL_RETURN_IF_NULL(task);
    OSAL_RETURN_IF_NULL(value);
    return os_task_notify_and_query_impl(task, bits, value);
}

/**
 * @brief OSAL 接口 osal_task_notify_wait。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_task_notify_wait(uint32_t clear_on_entry,
                                    uint32_t clear_on_exit, uint32_t *value,
                                    osal_tick_t timeout)
{
    return os_task_notify_wait_impl(clear_on_entry, clear_on_exit, value,
                                    timeout);
}

/**
 * @brief OSAL 接口 osal_task_notify_value_clear。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_task_notify_value_clear(uint32_t bits)
{
    return os_task_notify_value_clear_impl(bits);
}

/**
 * @brief OSAL 接口 osal_critical_enter。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_critical_enter(void)
{
    return os_critical_enter_impl();
}

/**
 * @brief OSAL 接口 osal_critical_exit。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_critical_exit(void)
{
    return os_critical_exit_impl();
}

/**
 * @brief OSAL 接口 osal_ms_to_ticks。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_tick_t osal_ms_to_ticks(uint32_t milliseconds)
{
    return os_ms_to_ticks_impl(milliseconds);
}
