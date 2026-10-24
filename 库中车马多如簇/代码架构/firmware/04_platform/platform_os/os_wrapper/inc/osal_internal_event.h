/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_internal_event.h
 *
 * @par dependencies
 * - osal_event.h
 *
 * @author YA
 *
 * @brief 头文件，用于实现本模块接口及相关功能。
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

#ifndef OSAL_INTERNAL_EVENT_H
#define OSAL_INTERNAL_EVENT_H

#include "osal_event.h"
/**
 * @brief 执行 os_event_group_create_impl 操作。
 *
 * @param[in] event : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_event_group_create_impl(osal_event_handle_t *event);
/**
 * @brief 执行 os_event_group_delete_impl 操作。
 *
 * @param[in] event : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_event_group_delete_impl(osal_event_handle_t event);
/**
 * @brief 执行 os_event_group_set_bits_impl 操作。
 *
 * @param[in] event : 函数调用参数。
 * @param[in] bits : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_event_group_set_bits_impl(osal_event_handle_t event, uint32_t bits);
/**
 * @brief 执行 os_event_group_clear_bits_impl 操作。
 *
 * @param[in] event : 函数调用参数。
 * @param[in] bits : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_event_group_clear_bits_impl(osal_event_handle_t event, uint32_t bits);
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
                                            uint32_t *actual_bits);

#endif /* OSAL_INTERNAL_EVENT_H */
