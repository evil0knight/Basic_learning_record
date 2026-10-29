/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_xxx_handler.h
 *
 * @par dependencies
 * - bsp_xxx_driver.h
 *
 * @author YA
 *
 * @brief 提供 XXX 设备 Handler 的基础接口。
 *
 * Processing flow:
 *
 * Handler 初始化 -> Driver 初始化 -> Handler 控制。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef FRIMWARE_BARE_BSP_XXX_HANDLER_H
#define FRIMWARE_BARE_BSP_XXX_HANDLER_H

#include <stdint.h>

#include "../driver/bsp_xxx_driver.h"

typedef struct
{
    bsp_xxx_driver_t *p_driver;
} bsp_xxx_handler_t;

/**
 * @brief 初始化 XXX Handler 和 Driver。
 * @param[in,out] p_handler XXX Handler 对象指针。
 * @param[in,out] p_driver XXX Driver 对象指针。
 * @param[in] p_port_ops 平台接口表指针。
 * @param[in] p_port_context 平台接口上下文指针。
 * @return bsp_xxx_status_t 操作状态。
 */
bsp_xxx_status_t bsp_xxx_handler_init(
    bsp_xxx_handler_t * const p_handler,
    bsp_xxx_driver_t * const p_driver,
    const bsp_xxx_port_ops_t * const p_port_ops,
    void * const p_port_context);

/**
 * @brief 向 XXX Driver 转发控制命令。
 * @param[in] p_handler XXX Handler 对象指针。
 * @param[in] index 设备索引。单设备 Handler 只接受 0。
 * @param[in] command 控制命令。
 * @param[in] p_argument 命令参数指针。
 * @return bsp_xxx_status_t 操作状态。
 */
bsp_xxx_status_t bsp_xxx_handler_control(
    bsp_xxx_handler_t * const p_handler,
    uint32_t index,
    uint32_t command,
    void * const p_argument);

#endif /* FRIMWARE_BARE_BSP_XXX_HANDLER_H */
