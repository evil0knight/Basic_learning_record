/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_key_driver.c
 * @par dependencies
 * - bsp_key_driver.h
 * - stddef.h
 * @author YA
 * @brief 实现按键平台绑定和有效电平转换。
 * Processing flow:
 * 平台读电平 -> 转换为按下状态 -> 返回读取状态。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#include "bsp_key_driver.h"
#include <stddef.h>

/**
 * @brief 检查并绑定平台读取接口。
 * @param[in,out] p_driver Driver 对象。
 * @param[in] p_ops 平台接口表。
 * @param[in] p_context 平台对象指针(包含GPIO端口和引脚等信息)
 * @param[in] b_active_high 有效电平。
 * @return en_key_status_t 初始化状态。
 */
en_key_status_t bsp_key_driver_init(st_key_driver_t *p_driver,
    const st_key_port_ops_t *p_ops, void *p_context, bool b_active_high)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_ops) ||
        (NULL == p_ops->pf_read_level))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if (p_driver->b_initialized)
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    p_driver->st_port_ops = *p_ops;
    p_driver->p_port_context = p_context;
    p_driver->b_active_high = b_active_high;
    p_driver->pf_read_pressed = bsp_key_driver_read_pressed;
    p_driver->b_initialized = true;
    return BSP_KEY_OK;
}

/**
 * @brief 根据按键的有效电平，把“高/低电平”转换成“按下/松开”
 * @param[in] p_driver Driver 对象。
 * @param[out] p_pressed 按下状态。
 * @return en_key_status_t 参数、状态或读取结果。
 */
en_key_status_t bsp_key_driver_read_pressed(
    const st_key_driver_t *p_driver, bool *p_pressed)
{
    bool b_high;
    en_key_status_t en_status;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_pressed))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if ((!p_driver->b_initialized) ||
        (NULL == p_driver->st_port_ops.pf_read_level))
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    en_status = p_driver->st_port_ops.pf_read_level(
        p_driver->p_port_context, &b_high);
    if (BSP_KEY_OK == en_status)
    {
        *p_pressed = (b_high == p_driver->b_active_high);
    }
    return en_status;
}

/**
 * @brief 清除 Driver 绑定。
 * @param[in,out] p_driver Driver 对象。
 * @return en_key_status_t 释放结果。
 */
en_key_status_t bsp_key_driver_deinit(st_key_driver_t *p_driver)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_driver)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    *p_driver = (st_key_driver_t){0};
    return BSP_KEY_OK;
}
