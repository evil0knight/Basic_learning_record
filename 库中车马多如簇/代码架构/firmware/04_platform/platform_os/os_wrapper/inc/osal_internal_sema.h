#ifndef OSAL_INTERNAL_SEMA_H
#define OSAL_INTERNAL_SEMA_H

#include "osal_sema.h"

osal_status_t os_sema_binary_create_impl(osal_sema_handle_t *semaphore);
osal_status_t os_sema_counting_create_impl(uint32_t maximum, uint32_t initial,
                                           osal_sema_handle_t *semaphore);
osal_status_t os_sema_delete_impl(osal_sema_handle_t semaphore);
osal_status_t os_sema_take_impl(osal_sema_handle_t semaphore,
                                osal_tick_t timeout);
osal_status_t os_sema_give_impl(osal_sema_handle_t semaphore);

#endif
