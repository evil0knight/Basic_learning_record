/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_wrapper_xxx.h
 *
 * @par dependencies
 * - stdbool.h
 * - stdint.h
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

#ifndef FRIMWARE_BARE_BSP_WRAPPER_XXX_H
#define FRIMWARE_BARE_BSP_WRAPPER_XXX_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BSP_WRAPPER_XXX_OK = 0,
    BSP_WRAPPER_XXX_ERROR = -1,
    BSP_WRAPPER_XXX_ERR_ARGUMENT = -2,
    BSP_WRAPPER_XXX_ERR_STATE = -3
} bsp_wrapper_xxx_status_t;

typedef struct
{
    bsp_wrapper_xxx_status_t (*init)(void *context);
    bsp_wrapper_xxx_status_t (*control)(void *context, uint32_t index,
                                        uint32_t command, void *argument);
    void *context;
} bsp_wrapper_xxx_ops_t;
/**
 * @brief 注册 BSP Wrapper 的操作接口。
 *
 * @param[in] ops : 操作函数表及其上下文指针。
 * @return bool : 注册成功返回 true，参数无效或重复注册返回 false。
 *
 */

bool bsp_wrapper_xxx_register(const bsp_wrapper_xxx_ops_t *ops);
/**
 * @brief 初始化已注册的 BSP Wrapper。
 *
 * @return bsp_wrapper_xxx_status_t : 底层初始化状态。
 *
 */
bsp_wrapper_xxx_status_t bsp_wrapper_xxx_init(void);
/**
 * @brief 向已注册的 BSP Wrapper 转发控制命令。
 *
 * @param[in] index : 设备或通道索引。
 * @param[in] command : 控制命令编码。
 * @param[in] argument : 控制命令参数指针。
 * @return bsp_wrapper_xxx_status_t : 控制操作状态。
 *
 */
bsp_wrapper_xxx_status_t bsp_wrapper_xxx_control(uint32_t index,
                                                 uint32_t command,
                                                 void *argument);

#endif
