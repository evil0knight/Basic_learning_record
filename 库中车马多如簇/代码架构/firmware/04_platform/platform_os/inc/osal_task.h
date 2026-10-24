/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_task.h
 *
 * @par dependencies
 * - osal_error.h
 * - osal_types.h
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

#ifndef OSAL_TASK_H
#define OSAL_TASK_H

#include "osal_error.h"
#include "osal_types.h"
/**
 * @brief 执行 osal_task_create 操作。
 *
 * @param[in] name : 函数调用参数。
 * @param[in] entry : 函数调用参数。
 * @param[in] stack_depth : 函数调用参数。
 * @param[in] priority : 函数调用参数。
 * @param[out] task : 函数调用参数。
 * @param[in] argument : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t osal_task_create(const char *name, osal_task_entry_t entry,
                               size_t stack_depth, osal_priority_t priority,
                               osal_task_handle_t *task, void *argument);
/**
 * @brief 执行 osal_task_delete 操作。
 *
 * @param[in] task : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_task_delete(osal_task_handle_t task);
/**
 * @brief 执行 osal_task_start_scheduler 操作。
 *
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_task_start_scheduler(void);
/**
 * @brief 执行 osal_task_suspend 操作。
 *
 * @param[in] task : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_task_suspend(osal_task_handle_t task);
/**
 * @brief 执行 osal_task_resume 操作。
 *
 * @param[in] task : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_task_resume(osal_task_handle_t task);
/**
 * @brief 执行 osal_task_delay 操作。
 *
 * @param[in] ticks : 函数调用参数。
 * @return void : 无返回值。
 *
 */
void osal_task_delay(osal_tick_t ticks);
/**
 * @brief 执行 osal_task_get_tick_count 操作。
 *
 * @return osal_tick_t : 函数执行结果。
 *
 */
osal_tick_t osal_task_get_tick_count(void);
/**
 * @brief 执行 osal_task_get_current 操作。
 *
 * @return osal_task_handle_t : 函数执行结果。
 *
 */
osal_task_handle_t osal_task_get_current(void);

/*
 * 任务通知：直接写目标任务的通知值，不需要额外内核对象。
 * 一个任务只有一个通知值，只能被指定的那一个任务等待，
 * 适合"中断 → 指定任务""任务 A → 任务 B"这类一对一事件。
 */

/* 置位通知值，任务上下文用 */
osal_status_t osal_task_notify(osal_task_handle_t task, uint32_t bits);
/* 置位通知值，中断上下文用 */
osal_status_t osal_task_notify_from_isr(osal_task_handle_t task,
                                        uint32_t bits);
/* 置位并回读目标任务置位后的通知值 */
osal_status_t osal_task_notify_and_query(osal_task_handle_t task,
                                         uint32_t bits, uint32_t *value);
/* 等通知值：进入时清 clear_on_entry 位，退出时清 clear_on_exit 位 */
osal_status_t osal_task_notify_wait(uint32_t clear_on_entry,
                                    uint32_t clear_on_exit, uint32_t *value,
                                    osal_tick_t timeout);
/* 清当前任务通知值里的指定位 */
osal_status_t osal_task_notify_value_clear(uint32_t bits);
/**
 * @brief 执行 osal_critical_enter 操作。
 *
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t osal_critical_enter(void);
/**
 * @brief 执行 osal_critical_exit 操作。
 *
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_critical_exit(void);
/**
 * @brief 执行 osal_ms_to_ticks 操作。
 *
 * @param[in] milliseconds : 函数调用参数。
 * @return osal_tick_t : 函数执行结果。
 *
 */
osal_tick_t osal_ms_to_ticks(uint32_t milliseconds);

#endif
