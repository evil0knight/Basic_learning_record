/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_key_driver.c
 *
 * @par dependencies
 * - bsp_key_driver.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 实现按键 Driver 的 GPIO 资源保存和有效电平转换。
 *
 * Processing flow:
 *
 * 参数检查 -> 保存 GPIO 资源 -> 挂载读取函数 -> 转换有效电平。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#include "bsp_key_driver.h"
#include <stddef.h>

/**
 * @brief 实例化按键 Driver，保存 GPIO 资源并挂载读取操作。
 * @param[in,out] p_driver : 按键 Driver 对象。
 * @param[in] p_port_ops : GPIO 平台接口表。
 * @param[in] u32_key_id : 按键 ID，透传给 pf_read_level。
 * @param[in] b_active_high : true 表示高电平按下。
 * @return en_key_status_t : 实例化结果。
 */
en_key_status_t bsp_key_driver_inst(
    bsp_key_driver_t *p_driver,
    const bsp_key_port_ops_t *p_port_ops,
    uint32_t u32_key_id,
    bool b_active_high)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_port_ops) ||
        (NULL == p_port_ops->pf_read_level))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if (true == p_driver->b_initialized)
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    p_driver->p_port_ops = p_port_ops;
    p_driver->u32_key_id = u32_key_id;
    p_driver->b_active_high = b_active_high;
    p_driver->pf_read_pressed = bsp_key_driver_read_pressed;
    p_driver->b_initialized = true;
    return BSP_KEY_OK;
}

/**
 * @brief 根据有效电平读取按键按下状态。
 * @param[in] p_driver : 按键 Driver 对象。
 * @param[out] p_pressed : 按键按下状态。
 * @return en_key_status_t : 读取结果。
 */
en_key_status_t bsp_key_driver_read_pressed(
    bsp_key_driver_t *p_driver,
    bool *p_pressed)
{
    bool b_high;
    en_key_status_t en_status;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_pressed))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if ((false == p_driver->b_initialized) ||
        (NULL == p_driver->p_port_ops) ||
        (NULL == p_driver->p_port_ops->pf_read_level))
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    en_status = p_driver->p_port_ops->pf_read_level(
        p_driver->u32_key_id, &b_high);
    if (BSP_KEY_OK == en_status)
    {
        *p_pressed = (b_high == p_driver->b_active_high);
    }
    return en_status;
}

/**
 * @brief 清除按键 Driver 实例状态。
 * @param[in,out] p_driver : 按键 Driver 对象。
 * @return en_key_status_t : 清除结果。
 */
en_key_status_t bsp_key_driver_deinit(bsp_key_driver_t *p_driver)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_driver)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    *p_driver = (bsp_key_driver_t){0};
    return BSP_KEY_OK;
}
