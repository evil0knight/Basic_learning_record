/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_macros.h
 *
 * @par dependencies
 * - 无。
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

#ifndef OSAL_MACROS_H
#define OSAL_MACROS_H

#define OSAL_RETURN_IF_NULL(pointer)             \
    do                                           \
    {                                            \
        if ((pointer) == NULL)                   \
        {                                        \
            return OSAL_ERR_INVALID_POINTER;     \
        }                                        \
    } while (0)

#define OSAL_RETURN_IF_FALSE(condition, error)   \
    do                                           \
    {                                            \
        if (!(condition))                        \
        {                                        \
            return (error);                      \
        }                                        \
    } while (0)

#endif
