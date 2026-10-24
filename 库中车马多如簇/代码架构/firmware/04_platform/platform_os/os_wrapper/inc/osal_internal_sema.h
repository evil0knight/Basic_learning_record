/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_internal_sema.h
 *
 * @par dependencies
 * - osal_sema.h
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

#ifndef OSAL_INTERNAL_SEMA_H
#define OSAL_INTERNAL_SEMA_H

#include "osal_sema.h"
/**
 * @brief 执行 os_sema_binary_create_impl 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_sema_binary_create_impl(osal_sema_handle_t *semaphore);
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
                                           osal_sema_handle_t *semaphore);
/**
 * @brief 执行 os_sema_delete_impl 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_sema_delete_impl(osal_sema_handle_t semaphore);
/**
 * @brief 执行 os_sema_take_impl 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_sema_take_impl(osal_sema_handle_t semaphore,
                                osal_tick_t timeout);
/**
 * @brief 执行 os_sema_give_impl 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_sema_give_impl(osal_sema_handle_t semaphore);

#endif
