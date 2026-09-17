#ifndef OSAL_TASK_H
#define OSAL_TASK_H

#include "osal_error.h"
#include "osal_types.h"

osal_status_t osal_task_create(const char *name, osal_task_entry_t entry,
                               size_t stack_depth, osal_priority_t priority,
                               osal_task_handle_t *task, void *argument);
osal_status_t osal_task_delete(osal_task_handle_t task);
osal_status_t osal_task_start_scheduler(void);
osal_status_t osal_task_suspend(osal_task_handle_t task);
osal_status_t osal_task_resume(osal_task_handle_t task);
void osal_task_delay(osal_tick_t ticks);
osal_tick_t osal_task_get_tick_count(void);
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

osal_status_t osal_critical_enter(void);
osal_status_t osal_critical_exit(void);
osal_tick_t osal_ms_to_ticks(uint32_t milliseconds);

#endif
