/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_internal_globaldefs.h
 *
 * @par dependencies
 * - string.h
 * - osal_config.h
 * - osal_error.h
 * - osal_macros.h
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

#ifndef OSAL_INTERNAL_GLOBALDEFS_H
#define OSAL_INTERNAL_GLOBALDEFS_H

#include <string.h>

#include "osal_config.h"
#include "osal_error.h"
#include "osal_macros.h"
#include "osal_types.h"
/**
 * @brief 执行 osal_name_is_valid 操作。
 *
 * @param[in] name : 函数调用参数。
 * @return static inline bool : 函数执行结果。
 *
 */

static inline bool osal_name_is_valid(const char *name)
{
    return (name != NULL) && (memchr(name, '\0', OSAL_NAME_MAX) != NULL);
}

#endif
