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
 * @brief 声明 XXX BSP Wrapper 的状态码、设备描述和对外接口。
 *
 * Processing flow:
 *
 * Adapter register(槽位, dev) -> 切当前 -> init / control 转发到当前槽位。
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

//******************************* Defines begin ******************************//
/* 允许同时存在的 XXX 器件数量；register 时的槽位上限。 */
#define BSP_WRAPPER_XXX_DRV_MAX_NUM   2U
//******************************** Defines end *******************************//

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
/* XXX 器件描述：一个 Adapter 填一份注册进来，context 由 Adapter 自行维护。 */
typedef struct
{
    bsp_wrapper_xxx_status_t (*init)(void *context);
    bsp_wrapper_xxx_status_t (*control)(void *context, uint32_t index,
                                        uint32_t command, void *argument);
    void *context;
} bsp_wrapper_xxx_dev_t;
//******************************* Typedefs end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 把一个 XXX 器件描述注册到指定槽位，并切换当前活跃槽位。
 *
 * Steps:
 *  1. 检查槽位和 dev 指针及 init、control 两项是否有效。
 *  2. 检查该槽位是否已被占用，未占用时保存器件描述并置位标志。
 *  3. 把当前活跃槽位切到本次注册的槽位。
 *
 * @param[in] index : 槽位索引，范围 0 ~ BSP_WRAPPER_XXX_DRV_MAX_NUM-1。
 * @param[in] dev : 器件描述 {init, control, context}。
 * @return bool : 注册成功返回 true，参数无效或槽位被占返回 false。
 *
 */
bool bsp_wrapper_xxx_register(uint32_t index, const bsp_wrapper_xxx_dev_t *dev);

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
bsp_wrapper_xxx_status_t bsp_wrapper_xxx_init(void);

/**
 * @brief 向当前活跃槽位转发控制命令。
 *
 * Steps:
 *  1. 检查当前槽位是否已注册。
 *  2. 调用当前器件的 control，透传子索引、命令和参数。
 *
 * @param[in] index : 器件内部子索引（如子通道、按键 ID）。
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
