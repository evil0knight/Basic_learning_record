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

osal_status_t os_task_create_impl(const osal_task_record_t *record,
                                  osal_task_handle_t *task);
osal_status_t os_task_delete_impl(osal_task_handle_t task);
osal_status_t os_task_start_scheduler_impl(void);
osal_status_t os_task_suspend_impl(osal_task_handle_t task);
osal_status_t os_task_resume_impl(osal_task_handle_t task);
void os_task_delay_impl(osal_tick_t ticks);
osal_tick_t os_task_get_tick_count_impl(void);
osal_task_handle_t os_task_get_current_impl(void);
osal_status_t os_critical_enter_impl(void);
osal_status_t os_critical_exit_impl(void);
osal_tick_t os_ms_to_ticks_impl(uint32_t milliseconds);

#endif
