/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_mutex.h
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

#ifndef OSAL_MUTEX_H
#define OSAL_MUTEX_H

#include "osal_error.h"
#include "osal_types.h"
/**
 * @brief 执行 osal_mutex_create 操作。
 *
 * @param[in] mutex : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t osal_mutex_create(osal_mutex_handle_t *mutex);
/**
 * @brief 执行 osal_mutex_delete 操作。
 *
 * @param[in] mutex : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_mutex_delete(osal_mutex_handle_t mutex);
/**
 * @brief 执行 osal_mutex_take 操作。
 *
 * @param[in] mutex : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_mutex_take(osal_mutex_handle_t mutex, osal_tick_t timeout);
/**
 * @brief 执行 osal_mutex_give 操作。
 *
 * @param[in] mutex : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_mutex_give(osal_mutex_handle_t mutex);

#endif
