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
osal_status_t osal_critical_enter(void);
osal_status_t osal_critical_exit(void);
osal_tick_t osal_ms_to_ticks(uint32_t milliseconds);

#endif
