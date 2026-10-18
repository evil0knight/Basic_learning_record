/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_event.c
 *
 * @par dependencies
 * - osal_internal_globaldefs.h
 * - osal_internal_event.h
 *
 * @author YA
 *
 * @brief OSAL 事件组接口实现。
 *
 * Processing flow:
 *
 * 参数检查 -> 调用 OS implementation 事件组接口 -> 返回 OSAL 状态。
 *
 * @version V1.0 2026-10-17
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#include "osal_internal_globaldefs.h"
#include "osal_internal_event.h"

/**
 * @brief 创建事件组。
 *
 * Steps:
 *  1. 检查输出句柄是否为空。
 *  2. 调用 OS implementation 创建事件组。
 *
 * @param[out] event : 返回创建后的事件组句柄。
 * @return osal_status_t : 创建结果。
 */
osal_status_t osal_event_group_create(osal_event_handle_t *event)
{
    OSAL_RETURN_IF_NULL(event);
    return os_event_group_create_impl(event);
}

/**
 * @brief 删除事件组。
 *
 * @param[in] event : 待删除的事件组句柄。
 * @return osal_status_t : 删除结果。
 */
osal_status_t osal_event_group_delete(osal_event_handle_t event)
{
    OSAL_RETURN_IF_NULL(event);
    return os_event_group_delete_impl(event);
}

/**
 * @brief 设置事件组中的事件位。
 *
 * @param[in] event : 事件组句柄。
 * @param[in] bits  : 要设置的事件位掩码，不能为 0。
 * @return osal_status_t : 设置结果。
 */
osal_status_t osal_event_group_set_bits(osal_event_handle_t event,
                                        uint32_t bits)
{
    OSAL_RETURN_IF_NULL(event);
    OSAL_RETURN_IF_FALSE(bits != 0U, OSAL_ERR_INVALID_ARGUMENT);
    return os_event_group_set_bits_impl(event, bits);
}

/**
 * @brief 清除事件组中的事件位。
 *
 * @param[in] event : 事件组句柄。
 * @param[in] bits  : 要清除的事件位掩码。
 * @return osal_status_t : 清除结果。
 */
osal_status_t osal_event_group_clear_bits(osal_event_handle_t event,
                                          uint32_t bits)
{
    OSAL_RETURN_IF_NULL(event);
    return os_event_group_clear_bits_impl(event, bits);
}

/**
 * @brief 等待事件组中的事件位满足条件。
 *
 * @param[in] event        : 事件组句柄。
 * @param[in] bits         : 要等待的事件位掩码，不能为 0。
 * @param[in] clear_on_exit: 返回前是否清除已满足的事件位。
 * @param[in] wait_all_bits: true 等待全部事件位，false 等待任一事件位。
 * @param[in] timeout      : 最大等待时间，单位由 OSAL tick 定义。
 * @param[out] actual_bits : 返回实际触发的事件位，可为 NULL。
 * @return osal_status_t : 等待结果。
 */
osal_status_t osal_event_group_wait_bits(osal_event_handle_t event,
                                         uint32_t bits,
                                         bool clear_on_exit,
                                         bool wait_all_bits,
                                         osal_tick_t timeout,
                                         uint32_t *actual_bits)
{
    OSAL_RETURN_IF_NULL(event);
    OSAL_RETURN_IF_FALSE(bits != 0U, OSAL_ERR_INVALID_ARGUMENT);
    return os_event_group_wait_bits_impl(event, bits, clear_on_exit,
                                         wait_all_bits, timeout, actual_bits);
}
