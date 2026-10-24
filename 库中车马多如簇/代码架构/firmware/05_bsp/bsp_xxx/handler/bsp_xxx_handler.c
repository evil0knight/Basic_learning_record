/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_xxx_handler.c
 *
 * @par dependencies
 * - bsp_xxx_handler.h
 *
 * @author YA
 *
 * @brief 提供 XXX 设备 Handler 的基础实现。
 *
 * Processing flow:
 *
 * 检查参数 -> Driver bind -> Driver init -> 回调挂载 -> 保存 Driver 实例。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#include "bsp_xxx_handler.h"

#include <stddef.h>

/* 由应用 ISR 文件定义，目标 HAL ISR 入口负责调用这些回调。 */
extern void (*pf_pin_interrupt_callback)(void *, void *);
extern void (*pf_dma_interrupt_callback)(void *, void *);

/**
 * @brief 注册 XXX 外部中断回调。
 * @param[in] pf_callback 中断回调函数指针。
 * @return bsp_xxx_status_t 注册结果。
 */
static bsp_xxx_status_t bsp_xxx_register_int_callback(
    bsp_xxx_isr_callback_t pf_callback)
{
    if (pf_callback == NULL)
    {
        return BSP_XXX_ERR_ARGUMENT;
    }

    pf_pin_interrupt_callback = pf_callback;
    return BSP_XXX_OK;
}

/**
 * @brief 注册 XXX DMA 中断回调。
 * @param[in] pf_callback 中断回调函数指针。
 * @return bsp_xxx_status_t 注册结果。
 */
static bsp_xxx_status_t bsp_xxx_register_dma_callback(
    bsp_xxx_isr_callback_t pf_callback)
{
    if (pf_callback == NULL)
    {
        return BSP_XXX_ERR_ARGUMENT;
    }

    pf_dma_interrupt_callback = pf_callback;
    return BSP_XXX_OK;
}

/**
 * @brief 初始化 XXX Handler、Driver 和中断回调挂载。
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

    status = bsp_xxx_driver_init(p_driver,
                                 bsp_xxx_register_int_callback,
                                 bsp_xxx_register_dma_callback);
    if (status != BSP_XXX_OK)
    {
        return status;
    }

    p_handler->p_driver = p_driver;
    return BSP_XXX_OK;
}

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
