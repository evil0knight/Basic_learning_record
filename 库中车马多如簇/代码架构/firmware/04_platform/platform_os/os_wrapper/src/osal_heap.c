#include "osal_internal_heap.h"

void *osal_heap_alloc(size_t size)
{
    return (size == 0U) ? NULL : os_heap_alloc_impl(size);
}

osal_status_t osal_heap_free(void *memory)
{
    if (memory == NULL)
    {
        return OSAL_ERR_INVALID_POINTER;
    }
    return os_heap_free_impl(memory);
}
