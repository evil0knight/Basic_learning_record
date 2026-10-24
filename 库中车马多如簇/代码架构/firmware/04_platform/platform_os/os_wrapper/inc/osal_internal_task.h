/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_internal_task.h
 *
 * @par dependencies
 * - osal_config.h
 * - osal_task.h
 *
 * @author YA
 *
 * @brief 头文件，用于实现本模块接口及相关功能。
 *
 * Processing flow:
 *
 * 参数检查 -> 执行模块操作 -> 返回处理结果。
 *
 * @version V1.0 2026-10-24
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef OSAL_INTERNAL_TASK_H
#define OSAL_INTERNAL_TASK_H

#include "osal_config.h"
#include "osal_task.h"

typedef struct
{
    char name[OSAL_NAME_MAX];
    osal_task_entry_t entry;
    size_t stack_depth;
    osal_priority_t priority;
    void *argument;
} osal_task_record_t;
/**
 * @brief 执行 os_task_create_impl 操作。
 *
 * @param[in] record : 函数调用参数。
 * @param[out] task : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_task_create_impl(const osal_task_record_t *record,
                                  osal_task_handle_t *task);
/**
 * @brief 执行 os_task_delete_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_task_delete_impl(osal_task_handle_t task);
/**
 * @brief 执行 os_task_start_scheduler_impl 操作。
 *
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_task_start_scheduler_impl(void);
/**
 * @brief 执行 os_task_suspend_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_task_suspend_impl(osal_task_handle_t task);
/**
 * @brief 执行 os_task_resume_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_task_resume_impl(osal_task_handle_t task);
/**
 * @brief 执行 os_task_delay_impl 操作。
 *
 * @param[in] ticks : 函数调用参数。
 * @return void : 无返回值。
 *
 */
void os_task_delay_impl(osal_tick_t ticks);
/**
 * @brief 执行 os_task_get_tick_count_impl 操作。
 *
 * @return osal_tick_t : 函数执行结果。
 *
 */
osal_tick_t os_task_get_tick_count_impl(void);
/**
 * @brief 执行 os_task_get_current_impl 操作。
 *
 * @return osal_task_handle_t : 函数执行结果。
 *
 */
osal_task_handle_t os_task_get_current_impl(void);
/**
 * @brief 执行 os_task_notify_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @param[in] bits : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_task_notify_impl(osal_task_handle_t task, uint32_t bits);
/**
 * @brief 执行 os_task_notify_from_isr_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @param[in] bits : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_task_notify_from_isr_impl(osal_task_handle_t task,
                                           uint32_t bits);
/**
 * @brief 执行 os_task_notify_and_query_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @param[in] bits : 函数调用参数。
 * @param[out] value : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_task_notify_and_query_impl(osal_task_handle_t task,
                                            uint32_t bits, uint32_t *value);
/**
 * @brief 执行 os_task_notify_wait_impl 操作。
 *
 * @param[in] clear_on_entry : 函数调用参数。
 * @param[in] clear_on_exit : 函数调用参数。
 * @param[out] value : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_task_notify_wait_impl(uint32_t clear_on_entry,
                                       uint32_t clear_on_exit,
                                       uint32_t *value, osal_tick_t timeout);
/**
 * @brief 执行 os_task_notify_value_clear_impl 操作。
 *
 * @param[in] bits : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_task_notify_value_clear_impl(uint32_t bits);
/**
 * @brief 执行 os_critical_enter_impl 操作。
 *
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_critical_enter_impl(void);
/**
 * @brief 执行 os_critical_exit_impl 操作。
 *
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_critical_exit_impl(void);
/**
 * @brief 执行 os_ms_to_ticks_impl 操作。
 *
 * @param[in] milliseconds : 函数调用参数。
 * @return osal_tick_t : 函数执行结果。
 *
 */
osal_tick_t os_ms_to_ticks_impl(uint32_t milliseconds);

#endif
