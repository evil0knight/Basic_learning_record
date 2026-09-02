#ifndef USER_ISR_HANDLERS_H
#define USER_ISR_HANDLERS_H

#include "osal.h"

typedef enum
{
    USER_ISR_EVENT_XXX = 0,
    USER_ISR_EVENT_MAX
} user_isr_event_t;

typedef void (*user_isr_callback_t)(user_isr_event_t event,
                                    void *context,
                                    void *argument);

osal_status_t user_isr_callback_register(user_isr_event_t event,
                                         user_isr_callback_t callback,
                                         void *argument);
void user_isr_dispatch(user_isr_event_t event, void *context);

#endif
