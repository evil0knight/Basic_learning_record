#include "osal_internal_globaldefs.h"
#include "osal_internal_task.h"

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

osal_status_t osal_task_delete(osal_task_handle_t task)
{
    return os_task_delete_impl(task);
}

osal_status_t osal_task_start_scheduler(void)
{
    return os_task_start_scheduler_impl();
}

osal_status_t osal_task_suspend(osal_task_handle_t task)
{
    return os_task_suspend_impl(task);
}

osal_status_t osal_task_resume(osal_task_handle_t task)
{
    OSAL_RETURN_IF_NULL(task);
    return os_task_resume_impl(task);
}

void osal_task_delay(osal_tick_t ticks)
{
    os_task_delay_impl(ticks);
}

osal_tick_t osal_task_get_tick_count(void)
{
    return os_task_get_tick_count_impl();
}

osal_task_handle_t osal_task_get_current(void)
{
    return os_task_get_current_impl();
}

osal_status_t osal_critical_enter(void)
{
    return os_critical_enter_impl();
}

osal_status_t osal_critical_exit(void)
{
    return os_critical_exit_impl();
}

osal_tick_t osal_ms_to_ticks(uint32_t milliseconds)
{
    return os_ms_to_ticks_impl(milliseconds);
}
