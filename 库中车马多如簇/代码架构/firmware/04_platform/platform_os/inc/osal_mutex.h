#ifndef OSAL_MUTEX_H
#define OSAL_MUTEX_H

#include "osal_error.h"
#include "osal_types.h"

osal_status_t osal_mutex_create(osal_mutex_handle_t *mutex);
osal_status_t osal_mutex_delete(osal_mutex_handle_t mutex);
osal_status_t osal_mutex_take(osal_mutex_handle_t mutex, osal_tick_t timeout);
osal_status_t osal_mutex_give(osal_mutex_handle_t mutex);

#endif
