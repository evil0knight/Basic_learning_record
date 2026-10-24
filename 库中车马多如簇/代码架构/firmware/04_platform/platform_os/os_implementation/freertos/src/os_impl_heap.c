/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file os_impl_heap.c
 *
 * @par dependencies
 * - osal_internal_heap.h
 * - os_freertos.h
 *
 * @author YA
 *
 * @brief 源文件，用于实现本模块接口及相关功能。
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

#include "osal_internal_heap.h"
#include "os_freertos.h"

void *os_heap_alloc_impl(size_t size)
{
    return OSAL_PORT_IS_IN_ISR() ? NULL : pvPortMalloc(size);
}
/**
 * @brief 执行 os_heap_free_impl 操作。
 *
 * @param[in] memory : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_heap_free_impl(void *memory)
{
    if (OSAL_PORT_IS_IN_ISR())
    {
        return OSAL_ERR_IN_ISR;
    }
    vPortFree(memory);
    return OSAL_SUCCESS;
}
