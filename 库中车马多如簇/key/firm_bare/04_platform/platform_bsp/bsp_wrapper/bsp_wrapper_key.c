/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_wrapper_key.c
 * @par dependencies
 * - bsp_wrapper_key.h
 * - stdbool.h
 * - stddef.h
 * @author YA
 * @brief 校验按键调用状态并转发 Adapter 操作。
 * Processing flow:
 * 参数检查 -> 设备描述中的操作函数 -> Adapter。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#include "bsp_wrapper_key.h"
#include <stdbool.h>
#include <stddef.h>

#define KEY_DRV_MAX_NUM 2U

static st_key_drv_t g_a_key_drv_dev[KEY_DRV_MAX_NUM];
static bool g_a_key_registered[KEY_DRV_MAX_NUM];
static uint32_t g_u32_current_key_dev_idx;

/**
 * @brief 获取当前设备描述，设备上下文由 Adapter 持有。
 * @param[out] p_dev 当前设备描述地址。
 * @return en_key_status_t 查询状态。
 */
static en_key_status_t bsp_wrapper_key_get_current(st_key_drv_t **p_dev)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_dev)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if ((KEY_DRV_MAX_NUM <= g_u32_current_key_dev_idx) ||
        (!g_a_key_registered[g_u32_current_key_dev_idx]))
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    *p_dev = &g_a_key_drv_dev[g_u32_current_key_dev_idx];
    return BSP_KEY_OK;
}

/**
 * @brief 将 Adapter 组装的设备描述复制到 Wrapper 槽位。
 * @param[in] u32_index 槽位索引。
 * @param[in] p_dev 带 user_data 和操作函数的设备描述。
 * @return en_key_status_t 注册状态。
 */
en_key_status_t bsp_wrapper_key_reg(
    uint32_t u32_index, const st_key_drv_t *p_dev)
{
    //-------------------------参数检查 begin-------------------------//
    if ((KEY_DRV_MAX_NUM <= u32_index) || (NULL == p_dev) ||
        (NULL == p_dev->p_user_data) ||
        (NULL == p_dev->pf_key_drv_scan) ||
        (NULL == p_dev->pf_key_drv_deinit) ||
        (NULL == p_dev->pf_key_drv_take_events) ||
        (NULL == p_dev->pf_key_drv_irq_notify))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if (g_a_key_registered[u32_index])
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    g_a_key_drv_dev[u32_index] = *p_dev;
    g_a_key_drv_dev[u32_index].u32_idx = u32_index;
    g_a_key_registered[u32_index] = true;
    g_u32_current_key_dev_idx = u32_index;
    return BSP_KEY_OK;
}

/**
 * @brief 调用当前设备的 Adapter 扫描函数。
 * @param[in] 无；检查当前设备挂载状态。
 * @return en_key_status_t 扫描状态。
 */
en_key_status_t bsp_wrapper_key_scan(void)
{
    st_key_drv_t *p_dev;
    en_key_status_t en_status;

    //-------------------------参数检查 begin-------------------------//
    en_status = bsp_wrapper_key_get_current(&p_dev);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    //--------------------------参数检查 end--------------------------//

    return p_dev->pf_key_drv_scan(p_dev);
}

/**
 * @brief 转发 ISR 引脚到当前设备的 Adapter 通知函数。
 * @param[in] u16_pin 单一 EXTI 引脚掩码。
 * @return en_key_status_t 通知状态。
 */
en_key_status_t bsp_wrapper_key_irq_notify(uint16_t u16_pin)
{
    st_key_drv_t *p_dev;
    en_key_status_t en_status;

    //-------------------------参数检查 begin-------------------------//
    if ((0U == u16_pin) || (0U != (u16_pin & (u16_pin - 1U))))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    en_status = bsp_wrapper_key_get_current(&p_dev);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    //--------------------------参数检查 end--------------------------//

    return p_dev->pf_key_drv_irq_notify(p_dev, u16_pin);
}

/**
 * @brief 调用 Adapter 解除挂载，成功后取消设备注册。
 * @param[in] 无；检查当前设备是否已注册。
 * @return en_key_status_t 释放结果，失败时可重试。
 */
en_key_status_t bsp_wrapper_key_deinit(void)
{
    st_key_drv_t *p_dev;
    en_key_status_t en_status;

    //-------------------------参数检查 begin-------------------------//
    en_status = bsp_wrapper_key_get_current(&p_dev);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    //--------------------------参数检查 end--------------------------//

    en_status = p_dev->pf_key_drv_deinit(p_dev);
    if (BSP_KEY_OK == en_status)
    {
        g_a_key_registered[p_dev->u32_idx] = false;
    }
    return en_status;
}

/**
 * @brief 通过设备描述取走指定按键的事件位。
 * @param[in] u32_key_id 组内按键 ID。
 * @param[out] p_flags 动作位集合，读取后清零。
 * @return en_key_status_t 查询状态。
 */
en_key_status_t bsp_wrapper_key_take_events(
    uint32_t u32_key_id, uint32_t *p_flags)
{
    st_key_drv_t *p_dev;
    en_key_status_t en_status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_flags)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    en_status = bsp_wrapper_key_get_current(&p_dev);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    //--------------------------参数检查 end--------------------------//

    return p_dev->pf_key_drv_take_events(p_dev, u32_key_id, p_flags);
}
