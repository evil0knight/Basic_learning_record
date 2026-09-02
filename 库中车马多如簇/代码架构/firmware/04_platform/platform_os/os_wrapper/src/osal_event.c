#include "osal_internal_globaldefs.h"
#include "osal_internal_event.h"

osal_status_t osal_event_group_create(osal_event_handle_t *event)
{
    OSAL_RETURN_IF_NULL(event);
    return os_event_group_create_impl(event);
}

osal_status_t osal_event_group_delete(osal_event_handle_t event)
{
    OSAL_RETURN_IF_NULL(event);
    return os_event_group_delete_impl(event);
}

osal_status_t osal_event_group_set_bits(osal_event_handle_t event, uint32_t bits)
{
    OSAL_RETURN_IF_NULL(event);
    OSAL_RETURN_IF_FALSE(bits != 0U, OSAL_ERR_INVALID_ARGUMENT);
    return os_event_group_set_bits_impl(event, bits);
}

osal_status_t osal_event_group_clear_bits(osal_event_handle_t event, uint32_t bits)
{
    OSAL_RETURN_IF_NULL(event);
    return os_event_group_clear_bits_impl(event, bits);
}

osal_status_t osal_event_group_wait_bits(osal_event_handle_t event,
                                         uint32_t bits,
                                         bool clear_on_exit,
                                         bool wait_all_bits,
                                         osal_tick_t timeout,
                                         uint32_t *actual_bits)
{
    OSAL_RETURN_IF_NULL(event);
    OSAL_RETURN_IF_FALSE(bits != 0U, OSAL_ERR_INVALID_ARGUMENT);
    return os_event_group_wait_bits_impl(event, bits, clear_on_exit,
                                         wait_all_bits, timeout, actual_bits);
}
