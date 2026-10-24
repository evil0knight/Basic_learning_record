/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file os_impl_queue.c
 *
 * @par dependencies
 * - osal_internal_queue.h
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

#include "osal_internal_queue.h"
#include "os_freertos.h"
/**
 * @brief 执行 os_queue_result 操作。
 *
 * @param[in] result : 函数调用参数。
 * @param[in] unavailable : 函数调用参数。
 * @return static osal_status_t : 函数执行结果。
 *
 */

static osal_status_t os_queue_result(BaseType_t result, osal_status_t unavailable)
{
    return (result == pdPASS) ? OSAL_SUCCESS : unavailable;
}
/**
 * @brief 执行 os_queue_create_impl 操作。
 *
 * @param[in] depth : 函数调用参数。
 * @param[in] item_size : 函数调用参数。
 * @param[in] queue : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_queue_create_impl(size_t depth, size_t item_size,
                                   osal_queue_handle_t *queue)
{
    QueueHandle_t native_queue;

    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    native_queue = xQueueCreate((UBaseType_t)depth, (UBaseType_t)item_size);
    if (native_queue == NULL)
    {
        return OSAL_ERR_NO_MEMORY;
    }
    *queue = (osal_queue_handle_t)native_queue;
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_queue_delete_impl 操作。
 *
 * @param[in] queue : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_queue_delete_impl(osal_queue_handle_t queue)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    vQueueDelete((QueueHandle_t)queue);
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_queue_send_impl 操作。
 *
 * @param[in] queue : 函数调用参数。
 * @param[in] data : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_queue_send_impl(osal_queue_handle_t queue, const void *data,
                                 osal_tick_t timeout)
{
    BaseType_t result;

    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = pdFALSE;
        result = xQueueSendFromISR((QueueHandle_t)queue, data, &should_yield);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        result = xQueueSend((QueueHandle_t)queue, data, osal_port_ticks(timeout));
    }
    return os_queue_result(result, OSAL_ERR_QUEUE_FULL);
}
/**
 * @brief 执行 os_queue_receive_impl 操作。
 *
 * @param[in] queue : 函数调用参数。
 * @param[in] data : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_queue_receive_impl(osal_queue_handle_t queue, void *data,
                                    osal_tick_t timeout)
{
    BaseType_t result;

    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = pdFALSE;
        result = xQueueReceiveFromISR((QueueHandle_t)queue, data, &should_yield);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        result = xQueueReceive((QueueHandle_t)queue, data, osal_port_ticks(timeout));
    }
    return os_queue_result(result, OSAL_ERR_QUEUE_EMPTY);
}
/**
 * @brief 执行 os_queue_peek_impl 操作。
 *
 * @param[in] queue : 函数调用参数。
 * @param[in] data : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_queue_peek_impl(osal_queue_handle_t queue, void *data,
                                 osal_tick_t timeout)
{
    BaseType_t result = OSAL_PORT_IS_IN_ISR()
                            ? xQueuePeekFromISR((QueueHandle_t)queue, data)
                            : xQueuePeek((QueueHandle_t)queue, data,
                                         osal_port_ticks(timeout));
    return os_queue_result(result, OSAL_ERR_QUEUE_EMPTY);
}
/**
 * @brief 执行 os_queue_messages_waiting_impl 操作。
 *
 * @param[in] queue : 函数调用参数。
 * @return size_t : 函数执行结果。
 *
 */

size_t os_queue_messages_waiting_impl(osal_queue_handle_t queue)
{
    UBaseType_t count = OSAL_PORT_IS_IN_ISR()
                            ? uxQueueMessagesWaitingFromISR((QueueHandle_t)queue)
                            : uxQueueMessagesWaiting((QueueHandle_t)queue);
    return (size_t)count;
}
