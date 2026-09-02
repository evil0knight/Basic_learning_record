#ifndef OSAL_INTERNAL_EVENT_H
#define OSAL_INTERNAL_EVENT_H

#include "osal_event.h"

osal_status_t os_event_group_create_impl(osal_event_handle_t *event);
osal_status_t os_event_group_delete_impl(osal_event_handle_t event);
osal_status_t os_event_group_set_bits_impl(osal_event_handle_t event, uint32_t bits);
osal_status_t os_event_group_clear_bits_impl(osal_event_handle_t event, uint32_t bits);
osal_status_t os_event_group_wait_bits_impl(osal_event_handle_t event,
                                            uint32_t bits,
                                            bool clear_on_exit,
                                            bool wait_all_bits,
                                            osal_tick_t timeout,
                                            uint32_t *actual_bits);

#endif /* OSAL_INTERNAL_EVENT_H */
