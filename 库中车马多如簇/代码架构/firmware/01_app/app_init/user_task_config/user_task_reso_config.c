#include "user_task_reso_config.h"

#include "rtos_config.h"

user_task_config_t g_user_tasks[USER_TASK_IDX_MAX] =
{
    {false, "xxx", NULL, CFG_TASK_XXX_STACK_SIZE,
     CFG_TASK_XXX_PRIORITY, NULL, NULL}
};

user_queue_config_t g_user_queues[USER_QUEUE_IDX_MAX] =
{
    {false, CFG_QUEUE_XXX_DEPTH, sizeof(uint8_t), NULL}
};

user_mutex_config_t g_user_mutexes[USER_MUTEX_IDX_MAX] =
{
    {false, NULL}
};

user_sema_config_t g_user_semaphores[USER_SEMA_IDX_MAX] =
{
    {false, true, CFG_SEMA_XXX_MAX_COUNT,
     CFG_SEMA_XXX_INIT_COUNT, NULL}
};

static void user_tasks_release(void)
{
    size_t index;

    for (index = 0U; index < USER_TASK_IDX_MAX; ++index)
    {
        if (g_user_tasks[index].handle != NULL)
        {
            (void)osal_task_delete(g_user_tasks[index].handle);
            g_user_tasks[index].handle = NULL;
        }
    }
}

void user_os_resources_deinit(void)
{
    size_t index;

    user_tasks_release();

    for (index = 0U; index < USER_SEMA_IDX_MAX; ++index)
    {
        if (g_user_semaphores[index].handle != NULL)
        {
            (void)osal_sema_delete(g_user_semaphores[index].handle);
            g_user_semaphores[index].handle = NULL;
        }
    }

    for (index = 0U; index < USER_MUTEX_IDX_MAX; ++index)
    {
        if (g_user_mutexes[index].handle != NULL)
        {
            (void)osal_mutex_delete(g_user_mutexes[index].handle);
            g_user_mutexes[index].handle = NULL;
        }
    }

    for (index = 0U; index < USER_QUEUE_IDX_MAX; ++index)
    {
        if (g_user_queues[index].handle != NULL)
        {
            (void)osal_queue_delete(g_user_queues[index].handle);
            g_user_queues[index].handle = NULL;
        }
    }
}

osal_status_t user_os_resources_init(void)
{
    osal_status_t status;
    size_t index;

    for (index = 0U; index < USER_QUEUE_IDX_MAX; ++index)
    {
        if (!g_user_queues[index].enabled)
        {
            continue;
        }
        status = osal_queue_create(g_user_queues[index].depth,
                                   g_user_queues[index].item_size,
                                   &g_user_queues[index].handle);
        if (status != OSAL_SUCCESS)
        {
            user_os_resources_deinit();
            return status;
        }
    }

    for (index = 0U; index < USER_MUTEX_IDX_MAX; ++index)
    {
        if (!g_user_mutexes[index].enabled)
        {
            continue;
        }
        status = osal_mutex_create(&g_user_mutexes[index].handle);
        if (status != OSAL_SUCCESS)
        {
            user_os_resources_deinit();
            return status;
        }
    }

    for (index = 0U; index < USER_SEMA_IDX_MAX; ++index)
    {
        if (!g_user_semaphores[index].enabled)
        {
            continue;
        }
        if (g_user_semaphores[index].binary)
        {
            status = osal_sema_binary_create(
                &g_user_semaphores[index].handle);
        }
        else
        {
            status = osal_sema_counting_create(
                g_user_semaphores[index].maximum,
                g_user_semaphores[index].initial,
                &g_user_semaphores[index].handle);
        }
        if (status != OSAL_SUCCESS)
        {
            user_os_resources_deinit();
            return status;
        }
    }

    return OSAL_SUCCESS;
}

osal_status_t user_tasks_start(void)
{
    osal_status_t status;
    size_t index;

    for (index = 0U; index < USER_TASK_IDX_MAX; ++index)
    {
        if (!g_user_tasks[index].enabled)
        {
            continue;
        }
        if ((g_user_tasks[index].name == NULL) ||
            (g_user_tasks[index].entry == NULL))
        {
            user_tasks_release();
            return OSAL_ERR_INVALID_ARGUMENT;
        }
        status = osal_task_create(g_user_tasks[index].name,
                                  g_user_tasks[index].entry,
                                  g_user_tasks[index].stack_depth,
                                  g_user_tasks[index].priority,
                                  &g_user_tasks[index].handle,
                                  g_user_tasks[index].argument);
        if (status != OSAL_SUCCESS)
        {
            user_tasks_release();
            return status;
        }
    }

    return OSAL_SUCCESS;
}

osal_task_handle_t user_task_get(user_task_idx_t index)
{
    return (index < USER_TASK_IDX_MAX) ? g_user_tasks[index].handle : NULL;
}

osal_queue_handle_t user_queue_get(user_queue_idx_t index)
{
    return (index < USER_QUEUE_IDX_MAX) ? g_user_queues[index].handle : NULL;
}

osal_mutex_handle_t user_mutex_get(user_mutex_idx_t index)
{
    return (index < USER_MUTEX_IDX_MAX) ? g_user_mutexes[index].handle : NULL;
}

osal_sema_handle_t user_sema_get(user_sema_idx_t index)
{
    return (index < USER_SEMA_IDX_MAX)
               ? g_user_semaphores[index].handle
               : NULL;
}
