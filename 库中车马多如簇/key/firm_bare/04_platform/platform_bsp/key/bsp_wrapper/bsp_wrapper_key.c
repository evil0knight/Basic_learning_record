/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_wrapper_key.c
 * @par dependencies
 * - bsp_wrapper_key.h
 * - stddef.h
 * @author YA
 * @brief 多槽位保存按键器件函数表，按"当前器件"直接转发到对应函数。
 * Processing flow:
 * reg 写槽位并切当前 -> init / scan / notify 走当前器件。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "bsp_wrapper_key.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//************************** Static Variables begin **************************//
static bsp_wrapper_key_dev_t g_a_bsp_wrapper_key_dev[BSP_WRAPPER_KEY_DRV_MAX_NUM];
static bool                  g_a_bsp_wrapper_key_registered[BSP_WRAPPER_KEY_DRV_MAX_NUM];
static uint32_t              g_u32_bsp_wrapper_key_current = 0U;
//*************************** Static Variables end ***************************//

//************************** Static Functions begin **************************//
/**
 * @brief 取当前器件，调用前须保证已注册。
 */
static bsp_wrapper_key_dev_t *bsp_wrapper_key_get_current(void)
{
    return &g_a_bsp_wrapper_key_dev[g_u32_bsp_wrapper_key_current];
}
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
bool bsp_wrapper_key_reg(uint32_t u32_index, bsp_wrapper_key_dev_t *p_dev)
{
    //-------------------------参数检查 begin-------------------------//
    if ((BSP_WRAPPER_KEY_DRV_MAX_NUM <= u32_index) || (NULL == p_dev) ||
        (NULL == p_dev->init) || (NULL == p_dev->scan) ||
        (NULL == p_dev->notify))
    {
        return false;
    }
    if (g_a_bsp_wrapper_key_registered[u32_index])
    {
        return false;
    }
    //--------------------------参数检查 end--------------------------//

    g_a_bsp_wrapper_key_dev[u32_index]          = *p_dev;
    g_a_bsp_wrapper_key_dev[u32_index].u32_idx  = u32_index;
    g_a_bsp_wrapper_key_registered[u32_index]   = true;
    g_u32_bsp_wrapper_key_current               = u32_index;
    return true;
}

en_key_status_t bsp_wrapper_key_init(void)
{
    bsp_wrapper_key_dev_t *p_dev;

    //-------------------------参数检查 begin-------------------------//
    if (!g_a_bsp_wrapper_key_registered[g_u32_bsp_wrapper_key_current])
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    p_dev = bsp_wrapper_key_get_current();
    return p_dev->init(p_dev);
}

en_key_status_t bsp_wrapper_key_scan(void)
{
    bsp_wrapper_key_dev_t *p_dev;

    //-------------------------参数检查 begin-------------------------//
    if (!g_a_bsp_wrapper_key_registered[g_u32_bsp_wrapper_key_current])
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    p_dev = bsp_wrapper_key_get_current();
    return p_dev->scan(p_dev);
}

en_key_status_t bsp_wrapper_key_notify(uint16_t u16_pin)
{
    bsp_wrapper_key_dev_t *p_dev;

    //-------------------------参数检查 begin-------------------------//
    if (!g_a_bsp_wrapper_key_registered[g_u32_bsp_wrapper_key_current])
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    p_dev = bsp_wrapper_key_get_current();
    return p_dev->notify(p_dev, u16_pin);
}
//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
