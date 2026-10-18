/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_mutex.c
 *
 * @par dependencies
 * - osal_internal_globaldefs.h
 * - 对应的 OSAL 内部接口头文件
 *
 * @author YA
 *
 * @brief OSAL 互斥量接口实现。
 *
 * 本文件负责参数检查，并将互斥量操作转发到 OS implementation 层。
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
#include "osal_internal_mutex.h"

/**
 * @brief OSAL 接口 osal_mutex_create。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_mutex_create(osal_mutex_handle_t *mutex)
{
    OSAL_RETURN_IF_NULL(mutex);
    return os_mutex_create_impl(mutex);
}

/**
 * @brief OSAL 接口 osal_mutex_delete。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_mutex_delete(osal_mutex_handle_t mutex)
{
    OSAL_RETURN_IF_NULL(mutex);
    return os_mutex_delete_impl(mutex);
}

/**
 * @brief OSAL 接口 osal_mutex_take。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_mutex_take(osal_mutex_handle_t mutex, osal_tick_t timeout)
{
    OSAL_RETURN_IF_NULL(mutex);
    return os_mutex_take_impl(mutex, timeout);
}

/**
 * @brief OSAL 接口 osal_mutex_give。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_mutex_give(osal_mutex_handle_t mutex)
{
    OSAL_RETURN_IF_NULL(mutex);
    return os_mutex_give_impl(mutex);
}
