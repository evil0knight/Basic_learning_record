/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_sema.c
 *
 * @par dependencies
 * - osal_internal_globaldefs.h
 * - 对应的 OSAL 内部接口头文件
 *
 * @author YA
 *
 * @brief OSAL 信号量接口实现。
 *
 * 本文件负责参数检查，并将信号量操作转发到 OS implementation 层。
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
#include "osal_internal_sema.h"

/**
 * @brief OSAL 接口 osal_sema_binary_create。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_sema_binary_create(osal_sema_handle_t *semaphore)
{
    OSAL_RETURN_IF_NULL(semaphore);
    return os_sema_binary_create_impl(semaphore);
}

/**
 * @brief OSAL 接口 osal_sema_counting_create。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_sema_counting_create(uint32_t maximum, uint32_t initial,
                                        osal_sema_handle_t *semaphore)
{
    OSAL_RETURN_IF_NULL(semaphore);
    OSAL_RETURN_IF_FALSE(maximum > 0U, OSAL_ERR_INVALID_ARGUMENT);
    OSAL_RETURN_IF_FALSE(initial <= maximum, OSAL_ERR_INVALID_ARGUMENT);
    return os_sema_counting_create_impl(maximum, initial, semaphore);
}

/**
 * @brief OSAL 接口 osal_sema_delete。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_sema_delete(osal_sema_handle_t semaphore)
{
    OSAL_RETURN_IF_NULL(semaphore);
    return os_sema_delete_impl(semaphore);
}

/**
 * @brief OSAL 接口 osal_sema_take。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_sema_take(osal_sema_handle_t semaphore, osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(semaphore);
    return os_sema_take_impl(semaphore, timeout);
}

/**
 * @brief OSAL 接口 osal_sema_give。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_sema_give(osal_sema_handle_t semaphore)
{
    OSAL_RETURN_IF_NULL(semaphore);
    return os_sema_give_impl(semaphore);
}
