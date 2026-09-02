#ifndef OSAL_INTERNAL_MUTEX_H
#define OSAL_INTERNAL_MUTEX_H

#include "osal_mutex.h"

osal_status_t os_mutex_create_impl(osal_mutex_handle_t *mutex);
osal_status_t os_mutex_delete_impl(osal_mutex_handle_t mutex);
osal_status_t os_mutex_take_impl(osal_mutex_handle_t mutex, osal_tick_t timeout);
osal_status_t os_mutex_give_impl(osal_mutex_handle_t mutex);

#endif
