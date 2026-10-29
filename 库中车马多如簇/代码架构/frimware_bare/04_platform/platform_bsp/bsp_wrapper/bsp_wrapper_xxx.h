/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_wrapper_xxx.h
 *
 * @par dependencies
 * - stdbool.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 声明 XXX BSP Wrapper 的状态码、操作表和对外接口。
 *
 * Processing flow:
 *
 * Adapter 注册操作表 -> Wrapper 保存表 -> 上层 init/control 转发到 Adapter。
 *
 * @version V1.0 2026-10-24
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef FRIMWARE_BARE_BSP_WRAPPER_XXX_H
#define FRIMWARE_BARE_BSP_WRAPPER_XXX_H

//****************************** Includes begin ******************************//
#include <stdbool.h>
#include <stdint.h>
//******************************* Includes end *******************************//

//******************************* Enums begin ********************************//
/* Wrapper 对外统一状态码，0 表示成功，负数表示失败。 */
typedef enum
{
    BSP_WRAPPER_XXX_OK = 0,
    BSP_WRAPPER_XXX_ERROR = -1,
    BSP_WRAPPER_XXX_ERR_ARGUMENT = -2,
    BSP_WRAPPER_XXX_ERR_STATE = -3
} bsp_wrapper_xxx_status_t;
//******************************** Enums end *********************************//

//****************************** Typedefs begin ******************************//
/* Adapter 向 Wrapper 注册的操作表，context 由 Adapter 自行维护。 */
typedef struct
{
    bsp_wrapper_xxx_status_t (*init)(void *context);
    bsp_wrapper_xxx_status_t (*control)(void *context, uint32_t index,
                                        uint32_t command, void *argument);
    void *context;
} bsp_wrapper_xxx_ops_t;
//******************************* Typedefs end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 注册 BSP Wrapper 的操作接口。
 *
 * Steps:
 *  1. 检查操作表指针及 init、control 两项是否为空。
 *  2. 检查是否已注册过，未注册时整体保存操作表。
 *
 * @param[in] ops : 操作函数表及其上下文指针。
 * @return bool : 注册成功返回 true，参数无效或重复注册返回 false。
 *
 */
bool bsp_wrapper_xxx_register(const bsp_wrapper_xxx_ops_t *ops);

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
bsp_wrapper_xxx_status_t bsp_wrapper_xxx_init(void);

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
                                                 void *argument);
//************************* Function Prototypes end **************************//

#endif
//******************************* End of File ********************************//
