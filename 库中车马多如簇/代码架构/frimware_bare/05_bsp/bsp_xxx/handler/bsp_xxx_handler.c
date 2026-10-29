/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_xxx_handler.c
 *
 * @par dependencies
 * - bsp_xxx_handler.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief XXX Handler 的初始化、控制转发实现。
 *
 * Processing flow:
 *
 * Handler 绑定平台接口，初始化 Driver 并保存 Driver 实例。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "bsp_xxx_handler.h"

#include <stddef.h>


/**
 * @brief 初始化 Handler 并绑定 Driver。
 * @param[in,out] p_handler Handler 实例。
 * @param[in,out] p_driver Driver 实例。
 * @param[in] p_port_ops 平台操作接口。
 * @param[in] p_port_context 平台接口上下文。
 * @return bsp_xxx_status_t 操作状态。
 */
bsp_xxx_status_t bsp_xxx_handler_init(
    bsp_xxx_handler_t * const p_handler,
    bsp_xxx_driver_t * const p_driver,
    const bsp_xxx_port_ops_t * const p_port_ops,
    void * const p_port_context)
{
    bsp_xxx_status_t status;

    if ((p_handler == NULL) || (p_driver == NULL) ||
        (p_port_ops == NULL))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }

    status = bsp_xxx_driver_bind(p_driver, p_port_ops, p_port_context);
    if (status != BSP_XXX_OK)
    {
        return status;
    }

    status = bsp_xxx_driver_init(p_driver);
    if (status != BSP_XXX_OK)
    {
        return status;
    }

    p_handler->p_driver = p_driver;
    return BSP_XXX_OK;
}


/**
 * @brief 转发 Handler 控制命令。
 * @param[in] p_handler Handler 实例。
 * @param[in] index 设备索引。
 * @param[in] command 控制命令。
 * @param[in] p_argument 命令参数。
 * @return bsp_xxx_status_t 操作状态。
 */
bsp_xxx_status_t bsp_xxx_handler_control(
    bsp_xxx_handler_t * const p_handler,
    uint32_t index,
    uint32_t command,
    void * const p_argument)
{
    if ((p_handler == NULL) || (index != 0U) ||
        (p_handler->p_driver == NULL))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }

    return bsp_xxx_driver_control(p_handler->p_driver,
                                  command,
                                  p_argument);
}





