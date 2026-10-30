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
 * @brief 提供 XXX Driver 的实例化、接口资源和操作函数。
 *
 * Processing flow:
 *
 * Adapter 组装接口 -> inst 保存资源并挂载操作 -> Handler 调用 Driver 操作。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef FRIMWARE_BARE_BSP_XXX_DRIVER_H
#define FRIMWARE_BARE_BSP_XXX_DRIVER_H

//****************************** Includes begin ******************************//
#include <stdbool.h>
#include <stdint.h>
//******************************* Includes end *******************************//

//******************************* Enums begin ********************************//
typedef enum
{
    BSP_XXX_OK = 0,
    BSP_XXX_ERROR = -1,
    BSP_XXX_ERR_ARGUMENT = -2,
    BSP_XXX_ERR_STATE = -3
} bsp_xxx_status_t;
//******************************** Enums end *********************************//

//****************************** Typedefs begin ******************************//
typedef struct bsp_xxx_driver bsp_xxx_driver_t;
typedef bsp_xxx_status_t (*bsp_xxx_driver_operation_t)(
    bsp_xxx_driver_t *p_driver,
    uint32_t command,
    void *p_argument);
typedef void (*bsp_xxx_driver_event_t)(
    bsp_xxx_driver_t *p_driver,
    void *p_data);
//******************************* Typedefs end *******************************//

//****************************** Structs begin *******************************//
typedef struct
{
    bsp_xxx_status_t (*pf_init)(void *p_context);
    bsp_xxx_status_t (*pf_deinit)(void *p_context);
    bsp_xxx_status_t (*pf_control)(
        void *p_context,
        uint32_t command,
        void *p_argument);
    bsp_xxx_status_t (*pf_critical_enter)(void);
    bsp_xxx_status_t (*pf_critical_exit)(void);
} bsp_xxx_port_ops_t;

struct bsp_xxx_driver
{
    const bsp_xxx_port_ops_t *p_port_ops;
    void *p_port_context;
    bool initialized;

    bsp_xxx_status_t (*pf_init)(bsp_xxx_driver_t *p_driver);
    bsp_xxx_status_t (*pf_deinit)(bsp_xxx_driver_t *p_driver);
    bsp_xxx_driver_operation_t pf_control;
    bsp_xxx_driver_event_t pf_int_event;
    bsp_xxx_driver_event_t pf_dma_event;
};
//******************************* Structs end ********************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 实例化 XXX Driver，保存接口资源并挂载操作函数。
 *
 * Steps:
 *  1. 检查 Driver、平台接口和上下文。
 *  2. 保存接口资源并挂载 Driver 操作函数。
 *  3. 调用平台初始化接口并标记 Driver 已初始化。
 *
 * @param[in,out] p_driver XXX Driver 对象。
 * @param[in] p_port_ops 平台操作接口表。
 * @param[in] p_port_context 平台上下文。
 * @return bsp_xxx_status_t : 实例化结果。
 */
bsp_xxx_status_t bsp_xxx_driver_inst(
    bsp_xxx_driver_t *p_driver,
    const bsp_xxx_port_ops_t *p_port_ops,
    void *p_port_context);

/**
 * @brief 处理 Platform 转发的设备中断事件。
 * @param[in] p_driver XXX Driver 对象。
 * @param[in] p_data 中断数据。
 * @return void : 无返回值。
 */
void bsp_xxx_int_interrupt_callback(
    bsp_xxx_driver_t *p_driver,
    void *p_data);

/**
 * @brief 处理 Platform 转发的 DMA 完成事件。
 * @param[in] p_driver XXX Driver 对象。
 * @param[in] p_data DMA 事件数据。
 * @return void : 无返回值。
 */
void bsp_xxx_dma_interrupt_callback(
    bsp_xxx_driver_t *p_driver,
    void *p_data);
//************************* Function Prototypes end **************************//

#endif /* FRIMWARE_BARE_BSP_XXX_DRIVER_H */
