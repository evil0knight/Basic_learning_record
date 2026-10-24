/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file os_impl_mutex.c
 *
 * @par dependencies
 * - osal_internal_mutex.h
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

#include "osal_internal_mutex.h"
#include "os_freertos.h"
/**
 * @brief 执行 os_mutex_create_impl 操作。
 *
 * @param[in] mutex : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_mutex_create_impl(osal_mutex_handle_t *mutex)
{
    SemaphoreHandle_t native_mutex;

    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    native_mutex = xSemaphoreCreateMutex();
    if (native_mutex == NULL)
    {
        return OSAL_ERR_NO_MEMORY;
    }
    *mutex = (osal_mutex_handle_t)native_mutex;
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_mutex_delete_impl 操作。
 *
 * @param[in] mutex : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_mutex_delete_impl(osal_mutex_handle_t mutex)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    vSemaphoreDelete((SemaphoreHandle_t)mutex);
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_mutex_take_impl 操作。
 *
 * @param[in] mutex : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_mutex_take_impl(osal_mutex_handle_t mutex, osal_tick_t timeout)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    return (xSemaphoreTake((SemaphoreHandle_t)mutex, osal_port_ticks(timeout)) == pdPASS)
               ? OSAL_SUCCESS
               : OSAL_ERR_TIMEOUT;
}
/**
 * @brief 执行 os_mutex_give_impl 操作。
 *
 * @param[in] mutex : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_mutex_give_impl(osal_mutex_handle_t mutex)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    return (xSemaphoreGive((SemaphoreHandle_t)mutex) == pdPASS)
               ? OSAL_SUCCESS
               : OSAL_ERROR;
}
