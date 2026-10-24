/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file os_impl_sema.c
 *
 * @par dependencies
 * - osal_internal_sema.h
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

#include "osal_internal_sema.h"
#include "os_freertos.h"
/**
 * @brief 执行 os_sema_binary_create_impl 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_sema_binary_create_impl(osal_sema_handle_t *semaphore)
{
    SemaphoreHandle_t native_semaphore;

    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    native_semaphore = xSemaphoreCreateBinary();
    if (native_semaphore == NULL)
    {
        return OSAL_ERR_NO_MEMORY;
    }
    *semaphore = (osal_sema_handle_t)native_semaphore;
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_sema_counting_create_impl 操作。
 *
 * @param[in] maximum : 函数调用参数。
 * @param[in] initial : 函数调用参数。
 * @param[in] semaphore : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_sema_counting_create_impl(uint32_t maximum, uint32_t initial,
                                           osal_sema_handle_t *semaphore)
{
    SemaphoreHandle_t native_semaphore;

    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    native_semaphore = xSemaphoreCreateCounting((UBaseType_t)maximum,
                                                (UBaseType_t)initial);
    if (native_semaphore == NULL)
    {
        return OSAL_ERR_NO_MEMORY;
    }
    *semaphore = (osal_sema_handle_t)native_semaphore;
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_sema_delete_impl 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_sema_delete_impl(osal_sema_handle_t semaphore)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    vSemaphoreDelete((SemaphoreHandle_t)semaphore);
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_sema_take_impl 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_sema_take_impl(osal_sema_handle_t semaphore,
                                osal_tick_t timeout)
{
    BaseType_t result;

    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = pdFALSE;
        result = xSemaphoreTakeFromISR((SemaphoreHandle_t)semaphore,
                                       &should_yield);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        result = xSemaphoreTake((SemaphoreHandle_t)semaphore,
                                osal_port_ticks(timeout));
    }
    return (result == pdPASS) ? OSAL_SUCCESS : OSAL_ERR_TIMEOUT;
}
/**
 * @brief 执行 os_sema_give_impl 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_sema_give_impl(osal_sema_handle_t semaphore)
{
    BaseType_t result;

    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = pdFALSE;
        result = xSemaphoreGiveFromISR((SemaphoreHandle_t)semaphore,
                                       &should_yield);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        result = xSemaphoreGive((SemaphoreHandle_t)semaphore);
    }
    return (result == pdPASS) ? OSAL_SUCCESS : OSAL_ERROR;
}
