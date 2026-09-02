#include "osal_internal_globaldefs.h"
#include "osal_internal_mutex.h"

osal_status_t osal_mutex_create(osal_mutex_handle_t *mutex)
{
    OSAL_RETURN_IF_NULL(mutex);
    return os_mutex_create_impl(mutex);
}

osal_status_t osal_mutex_delete(osal_mutex_handle_t mutex)
{
    OSAL_RETURN_IF_NULL(mutex);
    return os_mutex_delete_impl(mutex);
}

osal_status_t osal_mutex_take(osal_mutex_handle_t mutex, osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(mutex);
    return os_mutex_take_impl(mutex, timeout);
}

osal_status_t osal_mutex_give(osal_mutex_handle_t mutex)
{
    OSAL_RETURN_IF_NULL(mutex);
    return os_mutex_give_impl(mutex);
}
