#include "osal_internal_globaldefs.h"
#include "osal_internal_sema.h"

osal_status_t osal_sema_binary_create(osal_sema_handle_t *semaphore)
{
    OSAL_RETURN_IF_NULL(semaphore);
    return os_sema_binary_create_impl(semaphore);
}

osal_status_t osal_sema_counting_create(uint32_t maximum, uint32_t initial,
                                        osal_sema_handle_t *semaphore)
{
    OSAL_RETURN_IF_NULL(semaphore);
    OSAL_RETURN_IF_FALSE(maximum > 0U, OSAL_ERR_INVALID_ARGUMENT);
    OSAL_RETURN_IF_FALSE(initial <= maximum, OSAL_ERR_INVALID_ARGUMENT);
    return os_sema_counting_create_impl(maximum, initial, semaphore);
}

osal_status_t osal_sema_delete(osal_sema_handle_t semaphore)
{
    OSAL_RETURN_IF_NULL(semaphore);
    return os_sema_delete_impl(semaphore);
}

osal_status_t osal_sema_take(osal_sema_handle_t semaphore, osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(semaphore);
    return os_sema_take_impl(semaphore, timeout);
}

osal_status_t osal_sema_give(osal_sema_handle_t semaphore)
{
    OSAL_RETURN_IF_NULL(semaphore);
    return os_sema_give_impl(semaphore);
}
