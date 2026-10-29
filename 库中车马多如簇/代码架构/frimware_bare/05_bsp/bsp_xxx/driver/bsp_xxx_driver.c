/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_xxx_driver.c
 *
 * @par dependencies
 * - bsp_xxx_driver.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief XXX Driver 的平台绑定、初始化、控制和中断回调实现。
 *
 * Processing flow:
 *
 * Driver 校验平台接口并完成初始化，中断事件由 Platform 直接转发。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "bsp_xxx_driver.h"

#include <stddef.h>
/**
 * @brief 执行 bsp_xxx_int_interrupt_callback 操作。
 *
 * @param[in] p_object : 函数调用参数。
 * @param[in] p_data : 函数调用参数。
 * @return void : 函数执行结果。
 *
 */

void bsp_xxx_int_interrupt_callback(void *p_object, void *p_data)
{
    (void)p_object;
    (void)p_data;
}
/**
 * @brief 执行 bsp_xxx_dma_interrupt_callback 操作。
 *
 * @param[in] p_object : 函数调用参数。
 * @param[in] p_data : 函数调用参数。
 * @return void : 函数执行结果。
 *
 */

void bsp_xxx_dma_interrupt_callback(void *p_object, void *p_data)
{
    (void)p_object;
    (void)p_data;
}


/**
 * @brief 绑定平台操作接口。
 * @param[in,out] p_driver Driver 实例。
 * @param[in] p_port_ops 平台操作接口。
 * @param[in] p_port_context 平台接口上下文。
 * @return bsp_xxx_status_t 操作状态。
 */
bsp_xxx_status_t bsp_xxx_driver_bind(
    bsp_xxx_driver_t * const p_driver,
    const bsp_xxx_port_ops_t * const p_port_ops,
    void * const p_port_context)
{
    if ((p_driver == NULL) || (p_port_ops == NULL) ||
        (p_port_ops->pf_init == NULL) ||
        (p_port_ops->pf_control == NULL) ||
        (p_port_ops->pf_critical_enter == NULL) ||
        (p_port_ops->pf_critical_exit == NULL))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    if (p_driver->initialized)
    {
        return BSP_XXX_ERR_STATE;
    }

    p_driver->p_port_ops = p_port_ops;
    p_driver->p_port_context = p_port_context;
    return BSP_XXX_OK;
}


/**
 * @brief 初始化 Driver。
 * @param[in,out] p_driver Driver 实例。
 * @return bsp_xxx_status_t 操作状态。
 */
bsp_xxx_status_t bsp_xxx_driver_init(
    bsp_xxx_driver_t * const p_driver)
{
    bsp_xxx_status_t status;

    if ((p_driver == NULL) || (p_driver->p_port_ops == NULL))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    if (p_driver->initialized)
    {
        return BSP_XXX_OK;
    }

    status = p_driver->p_port_ops->pf_init(p_driver->p_port_context);
    if (status != BSP_XXX_OK)
    {
        return status;
    }

    p_driver->initialized = true;
    return BSP_XXX_OK;
}


/**
 * @brief 执行 Driver 控制命令。
 * @param[in] p_driver Driver 实例。
 * @param[in] command 控制命令。
 * @param[in] p_argument 命令参数。
 * @return bsp_xxx_status_t 操作状态。
 */
bsp_xxx_status_t bsp_xxx_driver_control(
    bsp_xxx_driver_t * const p_driver,
    uint32_t command,
    void * const p_argument)
{
    if ((p_driver == NULL) || !p_driver->initialized ||
        (p_driver->p_port_ops == NULL) ||
        (p_driver->p_port_ops->pf_control == NULL))
    {
        return BSP_XXX_ERR_STATE;
    }

    return p_driver->p_port_ops->pf_control(
        p_driver->p_port_context, command, p_argument);
}





