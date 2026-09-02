#include "osal_internal_mutex.h"
#include "os_freertos.h"

osal_status_t os_mutex_create_impl(osal_mutex_handle_t *mutex)
{
    SemaphoreHandle_t native_mutex;

    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    native_mutex = xSemaphoreCreateMutex();
    if (native_mutex == NULL)
    {
        return OSAL_ERR_NO_MEMORY;
    }
    *mutex = (osal_mutex_handle_t)native_mutex;
    return OSAL_SUCCESS;
}

osal_status_t os_mutex_delete_impl(osal_mutex_handle_t mutex)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    vSemaphoreDelete((SemaphoreHandle_t)mutex);
    return OSAL_SUCCESS;
}

osal_status_t os_mutex_take_impl(osal_mutex_handle_t mutex, osal_tick_t timeout)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    return (xSemaphoreTake((SemaphoreHandle_t)mutex, osal_port_ticks(timeout)) == pdPASS)
               ? OSAL_SUCCESS
               : OSAL_ERR_TIMEOUT;
}

osal_status_t os_mutex_give_impl(osal_mutex_handle_t mutex)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    return (xSemaphoreGive((SemaphoreHandle_t)mutex) == pdPASS)
               ? OSAL_SUCCESS
               : OSAL_ERROR;
}
