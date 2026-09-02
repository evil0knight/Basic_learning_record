#ifndef OSAL_SEMA_H
#define OSAL_SEMA_H

#include "osal_error.h"
#include "osal_types.h"

osal_status_t osal_sema_binary_create(osal_sema_handle_t *semaphore);
osal_status_t osal_sema_counting_create(uint32_t maximum, uint32_t initial,
                                        osal_sema_handle_t *semaphore);
osal_status_t osal_sema_delete(osal_sema_handle_t semaphore);
osal_status_t osal_sema_take(osal_sema_handle_t semaphore, osal_tick_t timeout);
osal_status_t osal_sema_give(osal_sema_handle_t semaphore);

#endif
