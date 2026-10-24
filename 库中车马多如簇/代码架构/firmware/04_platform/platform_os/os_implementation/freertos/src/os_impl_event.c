/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file os_impl_event.c
 *
 * @par dependencies
 * - osal_internal_event.h
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

#include "osal_internal_event.h"
#include "os_freertos.h"
/**
 * @brief 执行 os_event_group_create_impl 操作。
 *
 * @param[in] event : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_event_group_create_impl(osal_event_handle_t *event)
{
    EventGroupHandle_t native_event;

    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    native_event = xEventGroupCreate();
    if (native_event == NULL)
    {
        return OSAL_ERR_NO_MEMORY;
    }
    *event = (osal_event_handle_t)native_event;
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_event_group_delete_impl 操作。
 *
 * @param[in] event : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_event_group_delete_impl(osal_event_handle_t event)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    vEventGroupDelete((EventGroupHandle_t)event);
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_event_group_set_bits_impl 操作。
 *
 * @param[in] event : 函数调用参数。
 * @param[in] bits : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_event_group_set_bits_impl(osal_event_handle_t event, uint32_t bits)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        BaseType_t should_yield = pdFALSE;
        (void)xEventGroupSetBitsFromISR((EventGroupHandle_t)event,
                                        (EventBits_t)bits, &should_yield);
        portYIELD_FROM_ISR(should_yield);
    }
    else
    {
        (void)xEventGroupSetBits((EventGroupHandle_t)event, (EventBits_t)bits);
    }
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_event_group_clear_bits_impl 操作。
 *
 * @param[in] event : 函数调用参数。
 * @param[in] bits : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_event_group_clear_bits_impl(osal_event_handle_t event, uint32_t bits)
{
    (void)xEventGroupClearBits((EventGroupHandle_t)event, (EventBits_t)bits);
    return OSAL_SUCCESS;
}
/**
 * @brief 执行 os_event_group_wait_bits_impl 操作。
 *
 * @param[in] event : 函数调用参数。
 * @param[in] bits : 函数调用参数。
 * @param[in] clear_on_exit : 函数调用参数。
 * @param[in] wait_all_bits : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @param[in] actual_bits : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_event_group_wait_bits_impl(osal_event_handle_t event,
                                            uint32_t bits,
                                            bool clear_on_exit,
                                            bool wait_all_bits,
                                            osal_tick_t timeout,
                                            uint32_t *actual_bits)
{
    EventBits_t result;

    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    result = xEventGroupWaitBits((EventGroupHandle_t)event,
                                 (EventBits_t)bits,
                                 clear_on_exit ? pdTRUE : pdFALSE,
                                 wait_all_bits ? pdTRUE : pdFALSE,
                                 osal_port_ticks(timeout));
    if (actual_bits != NULL)
    {
        *actual_bits = (uint32_t)result;
    }

    if (wait_all_bits)
    {
        return ((result & bits) == bits) ? OSAL_SUCCESS : OSAL_ERR_TIMEOUT;
    }
    return ((result & bits) != 0U) ? OSAL_SUCCESS : OSAL_ERR_TIMEOUT;
}
