/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_queue.c
 *
 * @par dependencies
 * - osal_internal_globaldefs.h
 * - 对应的 OSAL 内部接口头文件
 *
 * @author YA
 *
 * @brief OSAL 消息队列接口实现。
 *
 * 本文件负责参数检查，并将队列操作转发到 OS implementation 层。
 *
 * Processing flow:
 *
 * 参数检查 -> 调用 OS implementation 接口 -> 返回 OSAL 状态。
 *
 * @version V1.0 2026-10-17
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#include "osal_internal_globaldefs.h"
#include "osal_internal_queue.h"

/**
 * @brief OSAL 接口 osal_queue_create。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_queue_create(size_t depth, size_t item_size,
                                osal_queue_handle_t *queue)
{
    OSAL_RETURN_IF_NULL(queue);
    OSAL_RETURN_IF_FALSE(depth > 0U, OSAL_ERR_INVALID_SIZE);
    OSAL_RETURN_IF_FALSE(item_size > 0U, OSAL_ERR_INVALID_SIZE);
    return os_queue_create_impl(depth, item_size, queue);
}

/**
 * @brief OSAL 接口 osal_queue_delete。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_queue_delete(osal_queue_handle_t queue)
{
    OSAL_RETURN_IF_NULL(queue);
    return os_queue_delete_impl(queue);
}

/**
 * @brief OSAL 接口 osal_queue_send。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_queue_send(osal_queue_handle_t queue, const void *data,
                              osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(queue);
    OSAL_RETURN_IF_NULL(data);
    return os_queue_send_impl(queue, data, timeout);
}

/**
 * @brief OSAL 接口 osal_queue_receive。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_queue_receive(osal_queue_handle_t queue, void *data,
                                 osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(queue);
    OSAL_RETURN_IF_NULL(data);
    return os_queue_receive_impl(queue, data, timeout);
}

/**
 * @brief OSAL 接口 osal_queue_peek。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_queue_peek(osal_queue_handle_t queue, void *data,
                              osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(queue);
    OSAL_RETURN_IF_NULL(data);
    return os_queue_peek_impl(queue, data, timeout);
}
/**
 * @brief 执行 osal_queue_messages_waiting 操作。
 *
 * @param[in] queue : 函数调用参数。
 * @return size_t : 函数执行结果。
 *
 */

size_t osal_queue_messages_waiting(osal_queue_handle_t queue)
{
    return (queue == NULL) ? 0U : os_queue_messages_waiting_impl(queue);
}
