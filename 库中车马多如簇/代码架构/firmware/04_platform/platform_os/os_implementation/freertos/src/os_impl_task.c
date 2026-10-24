/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file os_impl_task.c
 *
 * @par dependencies
 * - osal_internal_task.h
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

#include "osal_internal_task.h"
#include "os_freertos.h"
/**
 * @brief 执行 os_task_create_impl 操作。
 *
 * @param[in] record : 函数调用参数。
 * @param[out] task : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_task_create_impl(const osal_task_record_t *record,
                                  osal_task_handle_t *task)
{
    TaskHandle_t native_task = NULL;
    BaseType_t result;

    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }

    result = xTaskCreate(record->entry, record->name,
                         (configSTACK_DEPTH_TYPE)record->stack_depth,
                         record->argument, (UBaseType_t)record->priority,
                         &native_task);
    if (result != pdPASS)
    {
        return OSAL_ERR_NO_MEMORY;
    }
    *task = (osal_task_handle_t)native_task;
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_task_delete_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_task_delete_impl(osal_task_handle_t task)
{
#if (INCLUDE_vTaskDelete == 1)
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    vTaskDelete((TaskHandle_t)task);
    return OSAL_SUCCESS;
#else
    (void)task;
    return OSAL_ERROR;
#endif
}
/**
 * @brief 执行 os_task_start_scheduler_impl 操作。
 *
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_task_start_scheduler_impl(void)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    vTaskStartScheduler();
    return OSAL_ERROR;
}
/**
 * @brief 执行 os_task_suspend_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_task_suspend_impl(osal_task_handle_t task)
{
#if (INCLUDE_vTaskSuspend == 1)
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    vTaskSuspend((TaskHandle_t)task);
    return OSAL_SUCCESS;
#else
    (void)task;
    return OSAL_ERROR;
#endif
}
/**
 * @brief 执行 os_task_resume_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_task_resume_impl(osal_task_handle_t task)
{
#if (INCLUDE_vTaskSuspend == 1)
    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = xTaskResumeFromISR((TaskHandle_t)task);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        vTaskResume((TaskHandle_t)task);
    }
    return OSAL_SUCCESS;
#else
    (void)task;
    return OSAL_ERROR;
#endif
}
/**
 * @brief 执行 os_task_delay_impl 操作。
 *
 * @param[in] ticks : 函数调用参数。
 * @return void : 无返回值。
 *
 */

void os_task_delay_impl(osal_tick_t ticks)
{
#if (INCLUDE_vTaskDelay == 1)
    if (!OSAL_PORT_IS_IN_ISR())
    {
        vTaskDelay(osal_port_ticks(ticks));
    }
#else
    (void)ticks;
#endif
}
/**
 * @brief 执行 os_task_get_tick_count_impl 操作。
 *
 * @return osal_tick_t : 函数执行结果。
 *
 */

osal_tick_t os_task_get_tick_count_impl(void)
{
    TickType_t ticks = OSAL_PORT_IS_IN_ISR()
                           ? xTaskGetTickCountFromISR()
                           : xTaskGetTickCount();
    return (osal_tick_t)ticks;
}
/**
 * @brief 执行 os_task_get_current_impl 操作。
 *
 * @return osal_task_handle_t : 函数执行结果。
 *
 */

osal_task_handle_t os_task_get_current_impl(void)
{
#if (INCLUDE_xTaskGetCurrentTaskHandle == 1)
    return OSAL_PORT_IS_IN_ISR() ? NULL : (osal_task_handle_t)xTaskGetCurrentTaskHandle();
#else
    return NULL;
#endif
}
/**
 * @brief 执行 os_task_notify_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @param[in] bits : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_task_notify_impl(osal_task_handle_t task, uint32_t bits)
{
    BaseType_t result;

    if (task == NULL)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = pdFALSE;
        result = xTaskNotifyFromISR((TaskHandle_t)task, bits, eSetBits,
                                    &should_yield);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        result = xTaskNotify((TaskHandle_t)task, bits, eSetBits);
    }

    return (result == pdPASS) ? OSAL_SUCCESS : OSAL_ERROR;
}
/**
 * @brief 执行 os_task_notify_from_isr_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @param[in] bits : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_task_notify_from_isr_impl(osal_task_handle_t task,
                                           uint32_t bits)
{
    BaseType_t should_yield = pdFALSE;
    BaseType_t result;

    if (task == NULL)
    {
        return OSAL_ERR_INVALID_POINTER;
    }
    if (!OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERROR;
    }

    result = xTaskNotifyFromISR((TaskHandle_t)task, bits, eSetBits,
                                &should_yield);
    portYIELD_FROM_ISR(should_yield);

    return (result == pdPASS) ? OSAL_SUCCESS : OSAL_ERROR;
}
/**
 * @brief 执行 os_task_notify_and_query_impl 操作。
 *
 * @param[in] task : 函数调用参数。
 * @param[in] bits : 函数调用参数。
 * @param[out] value : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_task_notify_and_query_impl(osal_task_handle_t task,
                                            uint32_t bits, uint32_t *value)
{
    BaseType_t result;
    uint32_t notified_value = 0U;

    if (task == NULL)
    {
        return OSAL_ERR_INVALID_POINTER;
    }

    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = pdFALSE;
        result = xTaskNotifyAndQueryFromISR((TaskHandle_t)task, bits, eSetBits,
                                            &notified_value, &should_yield);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        result = xTaskNotifyAndQuery((TaskHandle_t)task, bits, eSetBits,
                                     &notified_value);
    }

    if (result != pdPASS)
    {
        return OSAL_ERROR;
    }
    if (value != NULL)
    {
        *value = notified_value;
    }

    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_task_notify_wait_impl 操作。
 *
 * @param[in] clear_on_entry : 函数调用参数。
 * @param[in] clear_on_exit : 函数调用参数。
 * @param[out] value : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_task_notify_wait_impl(uint32_t clear_on_entry,
                                       uint32_t clear_on_exit,
                                       uint32_t *value, osal_tick_t timeout)
{
    BaseType_t result;
    uint32_t notified_value = 0U;

    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }

    result = xTaskNotifyWait(clear_on_entry, clear_on_exit, &notified_value,
                             osal_port_ticks(timeout));
    if (result != pdPASS)
    {
        return OSAL_ERR_TIMEOUT;
    }
    if (value != NULL)
    {
        *value = notified_value;
    }

    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_task_notify_value_clear_impl 操作。
 *
 * @param[in] bits : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_task_notify_value_clear_impl(uint32_t bits)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }

    (void)ulTaskNotifyValueClear(NULL, bits);
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_critical_enter_impl 操作。
 *
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_critical_enter_impl(void)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    taskENTER_CRITICAL();
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_critical_exit_impl 操作。
 *
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_critical_exit_impl(void)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    taskEXIT_CRITICAL();
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_ms_to_ticks_impl 操作。
 *
 * @param[in] milliseconds : 函数调用参数。
 * @return osal_tick_t : 函数执行结果。
 *
 */

osal_tick_t os_ms_to_ticks_impl(uint32_t milliseconds)
{
    return (osal_tick_t)pdMS_TO_TICKS(milliseconds);
}
