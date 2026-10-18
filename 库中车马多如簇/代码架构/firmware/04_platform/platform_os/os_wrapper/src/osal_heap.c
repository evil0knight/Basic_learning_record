/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_heap.c
 *
 * @par dependencies
 * - osal_internal_globaldefs.h
 * - 对应的 OSAL 内部接口头文件
 *
 * @author YA
 *
 * @brief OSAL 堆内存接口实现。
 *
 * 本文件负责参数检查，并将堆操作转发到 OS implementation 层。
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

#include "osal_internal_heap.h"

/**
 * @brief 从 OSAL 堆中申请内存。
 * @param[in] size : 申请的字节数。
 * @return void * : 成功返回内存指针，失败返回 NULL。
 */
void *osal_heap_alloc(size_t size)
{
    return (size == 0U) ? NULL : os_heap_alloc_impl(size);
}

/**
 * @brief OSAL 接口 osal_heap_free。
 *
 * @param[in]  参数 : 由调用者提供的接口参数。
 * @return      : OSAL 操作结果或查询值。
 */
osal_status_t osal_heap_free(void *memory)
{
    if (memory == NULL)
    {
        return OSAL_ERR_INVALID_POINTER;
    }
    return os_heap_free_impl(memory);
}
