#ifndef OSAL_QUEUE_H
#define OSAL_QUEUE_H

#include "osal_error.h"
#include "osal_types.h"

osal_status_t osal_queue_create(size_t depth, size_t item_size,
                                osal_queue_handle_t *queue);
osal_status_t osal_queue_delete(osal_queue_handle_t queue);
osal_status_t osal_queue_send(osal_queue_handle_t queue, const void *data,
                              osal_tick_t timeout);
osal_status_t osal_queue_receive(osal_queue_handle_t queue, void *data,
                                 osal_tick_t timeout);
osal_status_t osal_queue_peek(osal_queue_handle_t queue, void *data,
                              osal_tick_t timeout);
size_t osal_queue_messages_waiting(osal_queue_handle_t queue);

#endif
