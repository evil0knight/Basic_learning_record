#include "osal_internal_globaldefs.h"
#include "osal_internal_queue.h"

osal_status_t osal_queue_create(size_t depth, size_t item_size,
                                osal_queue_handle_t *queue)
{
    OSAL_RETURN_IF_NULL(queue);
    OSAL_RETURN_IF_FALSE(depth > 0U, OSAL_ERR_INVALID_SIZE);
    OSAL_RETURN_IF_FALSE(item_size > 0U, OSAL_ERR_INVALID_SIZE);
    return os_queue_create_impl(depth, item_size, queue);
}

osal_status_t osal_queue_delete(osal_queue_handle_t queue)
{
    OSAL_RETURN_IF_NULL(queue);
    return os_queue_delete_impl(queue);
}

osal_status_t osal_queue_send(osal_queue_handle_t queue, const void *data,
                              osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(queue);
    OSAL_RETURN_IF_NULL(data);
    return os_queue_send_impl(queue, data, timeout);
}

osal_status_t osal_queue_receive(osal_queue_handle_t queue, void *data,
                                 osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(queue);
    OSAL_RETURN_IF_NULL(data);
    return os_queue_receive_impl(queue, data, timeout);
}

osal_status_t osal_queue_peek(osal_queue_handle_t queue, void *data,
                              osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(queue);
    OSAL_RETURN_IF_NULL(data);
    return os_queue_peek_impl(queue, data, timeout);
}

size_t osal_queue_messages_waiting(osal_queue_handle_t queue)
{
    return (queue == NULL) ? 0U : os_queue_messages_waiting_impl(queue);
}
