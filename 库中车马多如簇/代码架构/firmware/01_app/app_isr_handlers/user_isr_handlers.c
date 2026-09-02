#include "user_isr_handlers.h"

typedef struct
{
    user_isr_callback_t callback;
    void *argument;
} user_isr_slot_t;

static user_isr_slot_t g_user_isr_slots[USER_ISR_EVENT_MAX];

osal_status_t user_isr_callback_register(user_isr_event_t event,
                                         user_isr_callback_t callback,
                                         void *argument)
{
    if (event >= USER_ISR_EVENT_MAX)
    {
        return OSAL_ERR_INVALID_ARGUMENT;
    }

    g_user_isr_slots[event].callback = callback;
    g_user_isr_slots[event].argument = argument;
    return OSAL_SUCCESS;
}

void user_isr_dispatch(user_isr_event_t event, void *context)
{
    user_isr_callback_t callback;

    if (event >= USER_ISR_EVENT_MAX)
    {
        return;
    }

    callback = g_user_isr_slots[event].callback;
    if (callback != NULL)
    {
        callback(event, context, g_user_isr_slots[event].argument);
    }
}
