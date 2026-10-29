/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_xxx_driver.h
 *
 * @par dependencies
 * - stdbool.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 提供 XXX Driver 的绑定、初始化、控制和中断回调接口。
 *
 * Processing flow:
 *
 * 绑定 Port -> 初始化 Driver -> 执行控制或处理 Platform 转发的事件。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef FIRMWARE_BSP_XXX_DRIVER_H
#define FIRMWARE_BSP_XXX_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BSP_XXX_OK = 0,
    BSP_XXX_ERROR = -1,
    BSP_XXX_ERR_ARGUMENT = -2,
    BSP_XXX_ERR_STATE = -3
} bsp_xxx_status_t;


typedef struct
{
    bsp_xxx_status_t (*pf_init)(void *p_context);
    bsp_xxx_status_t (*pf_control)(void *p_context, uint32_t command,
                                   void *p_argument);
    bsp_xxx_status_t (*pf_critical_enter)(void);
    bsp_xxx_status_t (*pf_critical_exit)(void);
} bsp_xxx_port_ops_t;

typedef struct
{
    const bsp_xxx_port_ops_t *p_port_ops;
    void *p_port_context;
    bool initialized;
} bsp_xxx_driver_t;

/**
 * @brief 处理 Platform 转发的设备中断事件。
 * @param[in] p_object 设备对象。
 * @param[in] p_data 设备事件数据。
 * @return void 无返回值。
 */
void bsp_xxx_int_interrupt_callback(void *p_object, void *p_data);

/**
 * @brief 处理 Platform 转发的 DMA 完成事件。
 * @param[in] p_object 设备对象。
 * @param[in] p_data DMA 事件数据。
 * @return void 无返回值。
 */
void bsp_xxx_dma_interrupt_callback(void *p_object, void *p_data);

/**
 * @brief 将平台操作接口绑定到 XXX Driver。
 * @param[in,out] p_driver XXX Driver 对象指针。
 * @param[in] p_port_ops 平台操作接口表指针。
 * @param[in] p_port_context 平台操作上下文指针。
 * @return bsp_xxx_status_t 操作状态。
 */
bsp_xxx_status_t bsp_xxx_driver_bind(
    bsp_xxx_driver_t * const p_driver,
    const bsp_xxx_port_ops_t * const p_port_ops,
    void * const p_port_context);

/**
 * @brief 初始化 XXX Driver。
 * @param[in,out] p_driver XXX Driver 对象指针。
 * @return bsp_xxx_status_t 操作状态。
 */
bsp_xxx_status_t bsp_xxx_driver_init(
    bsp_xxx_driver_t * const p_driver);

/**
 * @brief 向已初始化的 XXX Driver 转发控制命令。
 * @param[in] p_driver XXX Driver 对象指针。
 * @param[in] command 控制命令。
 * @param[in] p_argument 命令参数指针。
 * @return bsp_xxx_status_t 操作状态。
 */
bsp_xxx_status_t bsp_xxx_driver_control(
    bsp_xxx_driver_t * const p_driver,
    uint32_t command,
    void * const p_argument);

#endif /* FIRMWARE_BSP_XXX_DRIVER_H */
