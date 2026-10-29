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
 * @brief 保存 Adapter 注册的操作表，并把上层 init/control 转发给 Adapter。
 *
 * Processing flow:
 *
 * 检查是否注册 -> 参数检查 -> 调用操作表函数 -> 返回统一状态。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

//****************************** Includes begin ******************************//
#include "bsp_wrapper_xxx.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//************************** Static Variables begin **************************//
/* Adapter 注册进来的操作表副本。 */
static bsp_wrapper_xxx_ops_t g_bsp_wrapper_xxx_ops;

/* 注册标志，保证同一时刻只有一个 Adapter 生效。 */
static bool g_bsp_wrapper_xxx_registered;
//*************************** Static Variables end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 注册 BSP Wrapper 的操作接口。
 *
 * Steps:
 *  1. 检查操作表指针及 init、control 两项是否为空。
 *  2. 检查是否已注册过，未注册时整体保存操作表并置位标志。
 *
 * @param[in] ops : 操作函数表及其上下文指针。
 * @return bool : 注册成功返回 true，参数无效或重复注册返回 false。
 *
 */
bool bsp_wrapper_xxx_register(const bsp_wrapper_xxx_ops_t *ops)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == ops) || (NULL == ops->init) || (NULL == ops->control) ||
        g_bsp_wrapper_xxx_registered)
    {
        return false;
    }
    //--------------------------参数检查 end--------------------------//

    g_bsp_wrapper_xxx_ops = *ops;
    g_bsp_wrapper_xxx_registered = true;
    return true;
}

/**
 * @brief 初始化已注册的 BSP Wrapper。
 *
 * Steps:
 *  1. 检查是否已有 Adapter 注册。
 *  2. 调用注册表中的 init，把上下文原样回传给 Adapter。
 *
 * @return bsp_wrapper_xxx_status_t : 底层初始化状态；未注册返回
 *         BSP_WRAPPER_XXX_ERR_STATE。
 *
 */
bsp_wrapper_xxx_status_t bsp_wrapper_xxx_init(void)
{
    //-------------------------参数检查 begin-------------------------//
    if (!g_bsp_wrapper_xxx_registered)
    {
        return BSP_WRAPPER_XXX_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    return g_bsp_wrapper_xxx_ops.init(g_bsp_wrapper_xxx_ops.context);
}

/**
 * @brief 向已注册的 BSP Wrapper 转发控制命令。
 *
 * Steps:
 *  1. 检查是否已有 Adapter 注册。
 *  2. 调用注册表中的 control，透传索引、命令和参数。
 *
 * @param[in] index : 设备或通道索引。
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
    //-------------------------参数检查 begin-------------------------//
    if (!g_bsp_wrapper_xxx_registered)
    {
        return BSP_WRAPPER_XXX_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    return g_bsp_wrapper_xxx_ops.control(g_bsp_wrapper_xxx_ops.context,
                                         index, command, argument);
}
//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
