#ifndef OSAL_INTERNAL_QUEUE_H
#define OSAL_INTERNAL_QUEUE_H

#include "osal_queue.h"

osal_status_t os_queue_create_impl(size_t depth, size_t item_size,
                                   osal_queue_handle_t *queue);
osal_status_t os_queue_delete_impl(osal_queue_handle_t queue);
osal_status_t os_queue_send_impl(osal_queue_handle_t queue, const void *data,
                                 osal_tick_t timeout);
osal_status_t os_queue_receive_impl(osal_queue_handle_t queue, void *data,
                                    osal_tick_t timeout);
osal_status_t os_queue_peek_impl(osal_queue_handle_t queue, void *data,
                                 osal_tick_t timeout);
size_t os_queue_messages_waiting_impl(osal_queue_handle_t queue);

#endif
