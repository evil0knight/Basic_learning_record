#include "osal_internal_heap.h"
#include "os_freertos.h"

void *os_heap_alloc_impl(size_t size)
{
    return OSAL_PORT_IS_IN_ISR() ? NULL : pvPortMalloc(size);
}

osal_status_t os_heap_free_impl(void *memory)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    vPortFree(memory);
    return OSAL_SUCCESS;
}
