#ifndef OSAL_TYPES_H
#define OSAL_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t osal_tick_t;
typedef uint32_t osal_priority_t;

typedef void *osal_task_handle_t;
typedef void *osal_queue_handle_t;
typedef void *osal_mutex_handle_t;
typedef void *osal_sema_handle_t;
typedef void *osal_timer_handle_t;
typedef void *osal_event_handle_t;

typedef void (*osal_task_entry_t)(void *argument);
typedef void (*osal_timer_callback_t)(osal_timer_handle_t timer, void *argument);

#define OSAL_WAIT_FOREVER UINT32_MAX

#endif
