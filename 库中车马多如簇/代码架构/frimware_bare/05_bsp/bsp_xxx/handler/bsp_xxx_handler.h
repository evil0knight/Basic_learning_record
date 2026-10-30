/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_xxx_handler.h
 *
 * @par dependencies
 * - bsp_xxx_driver.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 提供 XXX Handler 的实例化和控制接口。
 *
 * Processing flow:
 *
 * Adapter 提供资源 -> Handler 调用 Driver inst -> Handler 转发操作。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef FRIMWARE_BARE_BSP_XXX_HANDLER_H
#define FRIMWARE_BARE_BSP_XXX_HANDLER_H

//****************************** Includes begin ******************************//
#include <stdint.h>
#include "../driver/bsp_xxx_driver.h"
//******************************* Includes end *******************************//

//****************************** Structs begin *******************************//
typedef struct
{
    bsp_xxx_driver_t *p_driver;
} bsp_xxx_handler_t;
//******************************* Structs end ********************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 实例化 XXX Handler，并调用 Driver inst 保存资源和挂载操作。
 * @param[in,out] p_handler XXX Handler 对象。
 * @param[in,out] p_driver XXX Driver 对象。
 * @param[in] p_port_ops 平台接口表。
 * @param[in] p_port_context 平台上下文。
 * @return bsp_xxx_status_t : 实例化结果。
 */
bsp_xxx_status_t bsp_xxx_handler_inst(
    bsp_xxx_handler_t *p_handler,
    bsp_xxx_driver_t *p_driver,
    const bsp_xxx_port_ops_t *p_port_ops,
    void *p_port_context);

/**
 * @brief 转发 XXX 控制命令到 Driver 的挂载操作。
 * @param[in] p_handler XXX Handler 对象。
 * @param[in] index 单设备索引，只接受 0。
 * @param[in] command 控制命令。
 * @param[in] p_argument 命令参数。
 * @return bsp_xxx_status_t : 操作结果。
 */
bsp_xxx_status_t bsp_xxx_handler_control(
    bsp_xxx_handler_t *p_handler,
    uint32_t index,
    uint32_t command,
    void *p_argument);
//************************* Function Prototypes end **************************//

#endif /* FRIMWARE_BARE_BSP_XXX_HANDLER_H */