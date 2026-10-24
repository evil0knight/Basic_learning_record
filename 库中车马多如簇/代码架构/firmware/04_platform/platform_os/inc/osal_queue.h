/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_queue.h
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

#ifndef OSAL_QUEUE_H
#define OSAL_QUEUE_H

#include "osal_error.h"
#include "osal_types.h"
/**
 * @brief 执行 osal_queue_create 操作。
 *
 * @param[in] depth : 函数调用参数。
 * @param[in] item_size : 函数调用参数。
 * @param[in] queue : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t osal_queue_create(size_t depth, size_t item_size,
                                osal_queue_handle_t *queue);
/**
 * @brief 执行 osal_queue_delete 操作。
 *
 * @param[in] queue : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_queue_delete(osal_queue_handle_t queue);
/**
 * @brief 执行 osal_queue_send 操作。
 *
 * @param[in] queue : 函数调用参数。
 * @param[in] data : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_queue_send(osal_queue_handle_t queue, const void *data,
                              osal_tick_t timeout);
/**
 * @brief 执行 osal_queue_receive 操作。
 *
 * @param[in] queue : 函数调用参数。
 * @param[in] data : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_queue_receive(osal_queue_handle_t queue, void *data,
                                 osal_tick_t timeout);
/**
 * @brief 执行 osal_queue_peek 操作。
 *
 * @param[in] queue : 函数调用参数。
 * @param[in] data : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_queue_peek(osal_queue_handle_t queue, void *data,
                              osal_tick_t timeout);
/**
 * @brief 执行 osal_queue_messages_waiting 操作。
 *
 * @param[in] queue : 函数调用参数。
 * @return size_t : 函数执行结果。
 *
 */
size_t osal_queue_messages_waiting(osal_queue_handle_t queue);

#endif
