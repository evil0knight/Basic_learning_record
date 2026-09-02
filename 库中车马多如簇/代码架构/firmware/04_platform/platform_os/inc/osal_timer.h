#ifndef OSAL_TIMER_H
#define OSAL_TIMER_H

#include "osal_error.h"
#include "osal_types.h"

osal_status_t osal_timer_create(const char *name, osal_tick_t period,
                                bool auto_reload, osal_timer_callback_t callback,
                                void *argument, osal_timer_handle_t *timer);
osal_status_t osal_timer_start(osal_timer_handle_t timer, osal_tick_t timeout);
osal_status_t osal_timer_stop(osal_timer_handle_t timer, osal_tick_t timeout);
osal_status_t osal_timer_reset(osal_timer_handle_t timer, osal_tick_t timeout);
osal_status_t osal_timer_change_period(osal_timer_handle_t timer,
                                       osal_tick_t new_period,
                                       osal_tick_t timeout);
osal_status_t osal_timer_delete(osal_timer_handle_t timer, osal_tick_t timeout);
osal_tick_t osal_timer_get_period(osal_timer_handle_t timer);

#endif
