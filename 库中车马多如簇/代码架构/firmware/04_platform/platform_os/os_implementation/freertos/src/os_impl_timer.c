#include "osal_internal_heap.h"
#include "osal_internal_timer.h"
#include "os_freertos.h"

static void os_timer_callback(TimerHandle_t native_timer)
{
    osal_timer_record_t *record = pvTimerGetTimerID(native_timer);

    if ((record != NULL) && (record->callback != NULL))
    {
        record->callback((osal_timer_handle_t)record, record->argument);
    }
}

static osal_status_t os_timer_command_result(BaseType_t result)
{
    return (result == pdPASS) ? OSAL_SUCCESS : OSAL_ERROR;
}

static void os_timer_free_record(void *record, uint32_t unused)
{
    (void)unused;
    (void)os_heap_free_impl(record);
}

osal_status_t os_timer_create_impl(osal_timer_record_t *record)
{
    TimerHandle_t native_timer;

    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    native_timer = xTimerCreate(record->name, osal_port_ticks(record->period),
                                record->auto_reload ? pdTRUE : pdFALSE,
                                record, os_timer_callback);
    if (native_timer == NULL)
    {
        return OSAL_ERR_NO_MEMORY;
    }
    record->native_timer = native_timer;
    return OSAL_SUCCESS;
}

osal_status_t os_timer_start_impl(osal_timer_handle_t timer,
                                  osal_tick_t timeout)
{
    osal_timer_record_t *record = timer;
    BaseType_t result;

    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = pdFALSE;
        result = xTimerStartFromISR((TimerHandle_t)record->native_timer,
                                    &should_yield);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        result = xTimerStart((TimerHandle_t)record->native_timer,
                             osal_port_ticks(timeout));
    }
    return os_timer_command_result(result);
}

osal_status_t os_timer_stop_impl(osal_timer_handle_t timer,
                                 osal_tick_t timeout)
{
    osal_timer_record_t *record = timer;
    BaseType_t result;

    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = pdFALSE;
        result = xTimerStopFromISR((TimerHandle_t)record->native_timer,
                                   &should_yield);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        result = xTimerStop((TimerHandle_t)record->native_timer,
                            osal_port_ticks(timeout));
    }
    return os_timer_command_result(result);
}

osal_status_t os_timer_reset_impl(osal_timer_handle_t timer,
                                  osal_tick_t timeout)
{
    osal_timer_record_t *record = timer;
    BaseType_t result;

    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = pdFALSE;
        result = xTimerResetFromISR((TimerHandle_t)record->native_timer,
                                    &should_yield);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        result = xTimerReset((TimerHandle_t)record->native_timer,
                             osal_port_ticks(timeout));
    }
    return os_timer_command_result(result);
}

osal_status_t os_timer_change_period_impl(osal_timer_handle_t timer,
                                          osal_tick_t new_period,
                                          osal_tick_t timeout)
{
    osal_timer_record_t *record = timer;
    BaseType_t result;

    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = pdFALSE;
        result = xTimerChangePeriodFromISR((TimerHandle_t)record->native_timer,
                                           osal_port_ticks(new_period),
                                           &should_yield);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        result = xTimerChangePeriod((TimerHandle_t)record->native_timer,
                                    osal_port_ticks(new_period),
                                    osal_port_ticks(timeout));
    }
    if (result == pdPASS)
    {
        record->period = new_period;
    }
    return os_timer_command_result(result);
}

osal_status_t os_timer_delete_impl(osal_timer_handle_t timer,
                                   osal_tick_t timeout)
{
    osal_timer_record_t *record = timer;
    BaseType_t result;

    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    result = xTimerDelete((TimerHandle_t)record->native_timer,
                          osal_port_ticks(timeout));
    if (result != pdPASS)
    {
        return OSAL_ERROR;
    }
    result = xTimerPendFunctionCall(os_timer_free_record, record, 0U,
                                    osal_port_ticks(timeout));
    return os_timer_command_result(result);
}

osal_tick_t os_timer_get_period_impl(osal_timer_handle_t timer)
{
    const osal_timer_record_t *record = timer;
    return record->period;
}
