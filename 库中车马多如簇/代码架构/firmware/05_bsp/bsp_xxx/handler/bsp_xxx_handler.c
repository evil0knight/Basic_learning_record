/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
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
 * @brief 实现 XXX Handler 的实例化和控制转发。
 *
 * Processing flow:
 *
 * 参数检查 -> 调用 Driver inst -> 保存 Driver -> 转发操作。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
//****************************** Includes begin ******************************//
#include "bsp_xxx_handler.h"
#include <stddef.h>
//******************************* Includes end *******************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 实例化 XXX Handler，并调用 Driver inst。
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
    void *p_port_context)
{
    bsp_xxx_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_handler) || (NULL == p_driver) ||
        (NULL == p_port_ops))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_xxx_driver_inst(p_driver, p_port_ops, p_port_context);
    if (BSP_XXX_OK != status)
    {
        return status;
    }

    p_handler->p_driver = p_driver;
    return BSP_XXX_OK;
}

/**
 * @brief 转发 XXX 控制命令到 Driver 的挂载操作。
 * @param[in] p_handler XXX Handler 对象。
 * @param[in] index 单设备索引，只接受 0。
 * @param[in] command 控制命令。
 * @param[in] p_argument 控制参数。
 * @return bsp_xxx_status_t : 操作结果。
 */
bsp_xxx_status_t bsp_xxx_handler_control(
    bsp_xxx_handler_t *p_handler,
    uint32_t index,
    uint32_t command,
    void *p_argument)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_handler) || (0U != index) ||
        (NULL == p_handler->p_driver) ||
        (false == p_handler->p_driver->initialized) ||
        (NULL == p_handler->p_driver->pf_control))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return p_handler->p_driver->pf_control(
        p_handler->p_driver, command, p_argument);
}
//************************* Function Definitions end *************************//

//******************************* End of File ********************************//