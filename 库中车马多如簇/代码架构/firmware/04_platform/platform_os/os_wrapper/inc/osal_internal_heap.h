#ifndef OSAL_INTERNAL_HEAP_H
#define OSAL_INTERNAL_HEAP_H

#include "osal_heap.h"

void *os_heap_alloc_impl(size_t size);
osal_status_t os_heap_free_impl(void *memory);

#endif
