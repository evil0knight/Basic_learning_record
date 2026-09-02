#ifndef OSAL_HEAP_H
#define OSAL_HEAP_H

#include "osal_error.h"
#include "osal_types.h"

void *osal_heap_alloc(size_t size);
osal_status_t osal_heap_free(void *memory);

#endif
