/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
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
 * @brief 实现 XXX Driver 的实例化、资源保存和事件处理。
 *
 * Processing flow:
 *
 * 参数检查 -> 保存接口 -> 挂载操作 -> 初始化平台 -> 标记就绪。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
//****************************** Includes begin ******************************//
#include "bsp_xxx_driver.h"
#include <stddef.h>
//******************************* Includes end *******************************//

//************************** Static Functions begin **************************//
/**
 * @brief 调用平台初始化接口。
 *
 * @param[in] p_driver XXX Driver 对象。
 * @return bsp_xxx_status_t : 平台初始化结果。
 */
static bsp_xxx_status_t bsp_xxx_init_impl(bsp_xxx_driver_t *p_driver)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_driver->p_port_ops) ||
        (NULL == p_driver->p_port_ops->pf_init))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return p_driver->p_port_ops->pf_init(p_driver->p_port_context);
}

/**
 * @brief 调用平台反初始化接口。
 *
 * @param[in] p_driver XXX Driver 对象。
 * @return bsp_xxx_status_t : 平台反初始化结果。
 */
static bsp_xxx_status_t bsp_xxx_deinit_impl(bsp_xxx_driver_t *p_driver)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_driver->p_port_ops) ||
        (NULL == p_driver->p_port_ops->pf_deinit))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    p_driver->initialized = false;
    return p_driver->p_port_ops->pf_deinit(p_driver->p_port_context);
}

/**
 * @brief 调用平台控制接口。
 *
 * @param[in] p_driver XXX Driver 对象。
 * @param[in] command 控制命令编码。
 * @param[in] p_argument 控制命令参数，可为 NULL。
 * @return bsp_xxx_status_t : 平台控制结果。
 */
static bsp_xxx_status_t bsp_xxx_control_impl(
    bsp_xxx_driver_t *p_driver,
    uint32_t command,
    void *p_argument)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_driver->p_port_ops) ||
        (NULL == p_driver->p_port_ops->pf_control))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return p_driver->p_port_ops->pf_control(
        p_driver->p_port_context, command, p_argument);
}
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 实例化 XXX Driver，保存接口资源并挂载操作函数。
 *
 * Steps:
 *  1. 检查 Driver、平台接口和上下文。
 *  2. 保存平台资源，挂载初始化、反初始化、控制和事件函数。
 *  3. 调用平台初始化并标记 Driver 已初始化。
 *
 * @param[in,out] p_driver XXX Driver 对象。
 * @param[in] p_port_ops 平台操作接口表。
 * @param[in] p_port_context 平台上下文。
 * @return bsp_xxx_status_t : 实例化结果。
 */
bsp_xxx_status_t bsp_xxx_driver_inst(
    bsp_xxx_driver_t *p_driver,
    const bsp_xxx_port_ops_t *p_port_ops,
    void *p_port_context)
{
    bsp_xxx_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_port_ops) ||
        (NULL == p_port_ops->pf_init) ||
        (NULL == p_port_ops->pf_control))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    if (true == p_driver->initialized)
    {
        return BSP_XXX_ERR_STATE;
    }

    p_driver->p_port_ops = p_port_ops;
    p_driver->p_port_context = p_port_context;
    p_driver->pf_init = bsp_xxx_init_impl;
    p_driver->pf_deinit = bsp_xxx_deinit_impl;
    p_driver->pf_control = bsp_xxx_control_impl;
    p_driver->pf_int_event = bsp_xxx_int_interrupt_callback;
    p_driver->pf_dma_event = bsp_xxx_dma_interrupt_callback;

    status = p_driver->pf_init(p_driver);
    if (BSP_XXX_OK != status)
    {
        return status;
    }

    p_driver->initialized = true;
    return BSP_XXX_OK;
}

/**
 * @brief 处理 Platform 转发的设备中断事件。
 * @param[in] p_driver XXX Driver 对象。
 * @param[in] p_data 中断数据。
 * @return void : 无返回值。
 */
void bsp_xxx_int_interrupt_callback(
    bsp_xxx_driver_t *p_driver,
    void *p_data)
{
    (void)p_driver;
    (void)p_data;
}

/**
 * @brief 处理 Platform 转发的 DMA 完成事件。
 * @param[in] p_driver XXX Driver 对象。
 * @param[in] p_data DMA 事件数据。
 * @return void : 无返回值。
 */
void bsp_xxx_dma_interrupt_callback(
    bsp_xxx_driver_t *p_driver,
    void *p_data)
{
    (void)p_driver;
    (void)p_data;
}
//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
