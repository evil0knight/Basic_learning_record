/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_sema.h
 *
 * @par dependencies
 * - osal_error.h
 * - osal_types.h
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

#ifndef OSAL_SEMA_H
#define OSAL_SEMA_H

#include "osal_error.h"
#include "osal_types.h"
/**
 * @brief 执行 osal_sema_binary_create 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t osal_sema_binary_create(osal_sema_handle_t *semaphore);
/**
 * @brief 执行 osal_sema_counting_create 操作。
 *
 * @param[in] maximum : 函数调用参数。
 * @param[in] initial : 函数调用参数。
 * @param[in] semaphore : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_sema_counting_create(uint32_t maximum, uint32_t initial,
                                        osal_sema_handle_t *semaphore);
/**
 * @brief 执行 osal_sema_delete 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_sema_delete(osal_sema_handle_t semaphore);
/**
 * @brief 执行 osal_sema_take 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_sema_take(osal_sema_handle_t semaphore, osal_tick_t timeout);
/**
 * @brief 执行 osal_sema_give 操作。
 *
 * @param[in] semaphore : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_sema_give(osal_sema_handle_t semaphore);

#endif
