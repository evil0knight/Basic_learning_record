/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_config.h
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

#ifndef OSAL_CONFIG_H
#define OSAL_CONFIG_H

#define OSAL_FREERTOS 1

#ifndef OSAL_RTOS
#define OSAL_RTOS OSAL_FREERTOS
#endif

#ifndef OSAL_NAME_MAX
#define OSAL_NAME_MAX 16U
#endif

#endif
