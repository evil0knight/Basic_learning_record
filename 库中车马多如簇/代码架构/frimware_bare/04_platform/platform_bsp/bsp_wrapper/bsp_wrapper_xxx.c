/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_wrapper_xxx.c
 *
 * @par dependencies
 * - bsp_wrapper_xxx.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 多槽位保存 XXX 器件描述，按"当前槽位"转发 init 和 control。
 *
 * Processing flow:
 *
 * register 写槽位并切当前 -> init 用当前槽位 -> control 用当前槽位。
 *
 * @version V1.0 2026-10-24
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "bsp_wrapper_xxx.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//************************** Static Variables begin **************************//
/* 多槽位器件描述副本，一个 Adapter 占一个槽位。 */
static bsp_wrapper_xxx_dev_t g_a_bsp_wrapper_xxx_dev[BSP_WRAPPER_XXX_DRV_MAX_NUM];

/* 每个槽位是否已注册。 */
static bool g_a_bsp_wrapper_xxx_registered[BSP_WRAPPER_XXX_DRV_MAX_NUM];

/* 当前活跃槽位索引，register 时切换。 */
static uint32_t g_u32_bsp_wrapper_xxx_current = 0U;
//*************************** Static Variables end ***************************//

//************************** Static Functions begin **************************//
/**
 * @brief 取当前活跃槽位的器件描述，调用前须保证已注册。
 */
static const bsp_wrapper_xxx_dev_t *bsp_wrapper_xxx_get_current(void)
{
    return &g_a_bsp_wrapper_xxx_dev[g_u32_bsp_wrapper_xxx_current];
}
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 把 XXX 器件描述注册到指定槽位，并把当前槽位切到它。
 *
 * Steps:
 *  1. 检查槽位和 dev 指针及 init、control 两项是否有效。
 *  2. 检查该槽位是否已被占用，未占用时整体保存并置位标志。
 *  3. 把当前活跃槽位切到本次注册的槽位。
 *
 * @param[in] index : 槽位索引。
 * @param[in] dev : 器件描述。
 * @return bool : 注册成功返回 true，参数无效或槽位被占返回 false。
 *
 */
bool bsp_wrapper_xxx_register(uint32_t index, const bsp_wrapper_xxx_dev_t *dev)
{
    //-------------------------参数检查 begin-------------------------//
    if ((BSP_WRAPPER_XXX_DRV_MAX_NUM <= index) ||
        (NULL == dev) || (NULL == dev->init) || (NULL == dev->control))
    {
        return false;
    }
    if (g_a_bsp_wrapper_xxx_registered[index])
    {
        return false;
    }
    //--------------------------参数检查 end--------------------------//

    g_a_bsp_wrapper_xxx_dev[index]        = *dev;
    g_a_bsp_wrapper_xxx_registered[index] = true;
    g_u32_bsp_wrapper_xxx_current         = index;
    return true;
}

/**
 * @brief 初始化当前活跃槽位的 XXX 器件。
 *
 * Steps:
 *  1. 检查当前槽位是否已注册。
 *  2. 调用当前器件的 init，把 context 原样回传给 Adapter。
 *
 * @return bsp_wrapper_xxx_status_t : 底层初始化状态；未注册返回
 *         BSP_WRAPPER_XXX_ERR_STATE。
 *
 */
bsp_wrapper_xxx_status_t bsp_wrapper_xxx_init(void)
{
    const bsp_wrapper_xxx_dev_t *dev;

    //-------------------------参数检查 begin-------------------------//
    if (!g_a_bsp_wrapper_xxx_registered[g_u32_bsp_wrapper_xxx_current])
    {
        return BSP_WRAPPER_XXX_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    dev = bsp_wrapper_xxx_get_current();
    return dev->init(dev->context);
}

/**
 * @brief 向当前活跃槽位转发控制命令。
 *
 * Steps:
 *  1. 检查当前槽位是否已注册。
 *  2. 调用当前器件的 control，透传子索引、命令和参数。
 *
 * @param[in] index : 器件内部子索引。
 * @param[in] command : 控制命令编码。
 * @param[in] argument : 控制命令参数指针，可为 NULL。
 * @return bsp_wrapper_xxx_status_t : 控制操作状态；未注册返回
 *         BSP_WRAPPER_XXX_ERR_STATE。
 *
 */
bsp_wrapper_xxx_status_t bsp_wrapper_xxx_control(uint32_t index,
                                                 uint32_t command,
                                                 void *argument)
{
    const bsp_wrapper_xxx_dev_t *dev;

    //-------------------------参数检查 begin-------------------------//
    if (!g_a_bsp_wrapper_xxx_registered[g_u32_bsp_wrapper_xxx_current])
    {
        return BSP_WRAPPER_XXX_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    dev = bsp_wrapper_xxx_get_current();
    return dev->control(dev->context, index, command, argument);
}
//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
