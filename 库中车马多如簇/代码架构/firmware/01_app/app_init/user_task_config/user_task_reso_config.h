#ifndef USER_TASK_RESO_CONFIG_H
#define USER_TASK_RESO_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "osal.h"

typedef enum
{
    USER_TASK_IDX_XXX = 0,
    USER_TASK_IDX_MAX
} user_task_idx_t;

typedef enum
{
    USER_QUEUE_IDX_XXX = 0,
    USER_QUEUE_IDX_MAX
} user_queue_idx_t;

typedef enum
{
    USER_MUTEX_IDX_XXX = 0,
    USER_MUTEX_IDX_MAX
} user_mutex_idx_t;

typedef enum
{
    USER_SEMA_IDX_XXX = 0,
    USER_SEMA_IDX_MAX
} user_sema_idx_t;

typedef struct
{
    bool enabled;
    const char *name;
    osal_task_entry_t entry;
    size_t stack_depth;
    osal_priority_t priority;
    void *argument;
    osal_task_handle_t handle;
} user_task_config_t;

typedef struct
{
    bool enabled;
    size_t depth;
    size_t item_size;
    osal_queue_handle_t handle;
} user_queue_config_t;

typedef struct
{
    bool enabled;
    osal_mutex_handle_t handle;
} user_mutex_config_t;

typedef struct
{
    bool enabled;
    bool binary;
    uint32_t maximum;
    uint32_t initial;
    osal_sema_handle_t handle;
} user_sema_config_t;

extern user_task_config_t g_user_tasks[USER_TASK_IDX_MAX];
extern user_queue_config_t g_user_queues[USER_QUEUE_IDX_MAX];
extern user_mutex_config_t g_user_mutexes[USER_MUTEX_IDX_MAX];
extern user_sema_config_t g_user_semaphores[USER_SEMA_IDX_MAX];

osal_status_t user_os_resources_init(void);
osal_status_t user_tasks_start(void);
void user_os_resources_deinit(void);

osal_task_handle_t user_task_get(user_task_idx_t index);
osal_queue_handle_t user_queue_get(user_queue_idx_t index);
osal_mutex_handle_t user_mutex_get(user_mutex_idx_t index);
osal_sema_handle_t user_sema_get(user_sema_idx_t index);

#endif
