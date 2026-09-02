#ifndef OSAL_INTERNAL_TIMER_H
#define OSAL_INTERNAL_TIMER_H

#include "osal_config.h"
#include "osal_timer.h"

typedef struct
{
    char name[OSAL_NAME_MAX];
    osal_tick_t period;
    bool auto_reload;
    osal_timer_callback_t callback;
    void *argument;
    void *native_timer;
} osal_timer_record_t;

osal_status_t os_timer_create_impl(osal_timer_record_t *record);
osal_status_t os_timer_start_impl(osal_timer_handle_t timer,
                                  osal_tick_t timeout);
osal_status_t os_timer_stop_impl(osal_timer_handle_t timer,
                                 osal_tick_t timeout);
osal_status_t os_timer_reset_impl(osal_timer_handle_t timer,
                                  osal_tick_t timeout);
osal_status_t os_timer_change_period_impl(osal_timer_handle_t timer,
                                          osal_tick_t new_period,
                                          osal_tick_t timeout);
osal_status_t os_timer_delete_impl(osal_timer_handle_t timer,
                                   osal_tick_t timeout);
osal_tick_t os_timer_get_period_impl(osal_timer_handle_t timer);

#endif
