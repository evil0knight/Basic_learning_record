/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file os_impl_timer.c
 *
 * @par dependencies
 * - osal_internal_heap.h
 * - osal_internal_timer.h
 * - os_freertos.h
 *
 * @author YA
 *
 * @brief 源文件，用于实现本模块接口及相关功能。
 *
 * Processing flow:
 *
 * 参数检查 -> 执行模块操作 -> 返回处理结果。
 *
 * @version V1.0 2026-10-24
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#include "osal_internal_heap.h"
#include "osal_internal_timer.h"
#include "os_freertos.h"
/**
 * @brief 执行 os_timer_callback 操作。
 *
 * @param[in] native_timer : 函数调用参数。
 * @return static void : 函数执行结果。
 *
 */

static void os_timer_callback(TimerHandle_t native_timer)
{
    osal_timer_record_t *record = pvTimerGetTimerID(native_timer);

    if ((record != NULL) && (record->callback != NULL))
    {
        record->callback((osal_timer_handle_t)record, record->argument);
    }
}
/**
 * @brief 执行 os_timer_command_result 操作。
 *
 * @param[in] result : 函数调用参数。
 * @return static osal_status_t : 函数执行结果。
 *
 */

static osal_status_t os_timer_command_result(BaseType_t result)
{
    return (result == pdPASS) ? OSAL_SUCCESS : OSAL_ERROR;
}
/**
 * @brief 执行 os_timer_free_record 操作。
 *
 * @param[in] record : 函数调用参数。
 * @param[in] unused : 函数调用参数。
 * @return static void : 函数执行结果。
 *
 */

static void os_timer_free_record(void *record, uint32_t unused)
{
    (void)unused;
    (void)os_heap_free_impl(record);
}
/**
 * @brief 执行 os_timer_create_impl 操作。
 *
 * @param[in] record : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

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
/**
 * @brief 执行 os_timer_start_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

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
/**
 * @brief 执行 os_timer_stop_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

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
/**
 * @brief 执行 os_timer_reset_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

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
/**
 * @brief 执行 os_timer_change_period_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] new_period : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

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
/**
 * @brief 执行 os_timer_delete_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

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
/**
 * @brief 执行 os_timer_get_period_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @return osal_tick_t : 函数执行结果。
 *
 */

osal_tick_t os_timer_get_period_impl(osal_timer_handle_t timer)
{
    const osal_timer_record_t *record = timer;
    return record->period;
}
