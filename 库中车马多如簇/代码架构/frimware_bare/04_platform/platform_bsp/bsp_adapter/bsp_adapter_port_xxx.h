/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_adapter_port_xxx.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief 声明 XXX Adapter 的注册入口，供平台初始化流程调用。
 *
 * Processing flow:
 *
 * 目标工程初始化 -> bsp_adapter_xxx_register() -> Wrapper 取得操作表。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef FRIMWARE_BARE_BSP_ADAPTER_PORT_XXX_H
#define FRIMWARE_BARE_BSP_ADAPTER_PORT_XXX_H

//****************************** Includes begin ******************************//
#include <stdint.h>
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 将 XXX Adapter 注册到 BSP Wrapper。
 *
 * Steps:
 *  1. 组装 Adapter 的 init、control 和 context 三项操作表。
 *  2. 调用 bsp_wrapper_xxx_register() 挂载操作表。
 *
 * @return void : 无返回值；注册结果由 Wrapper 内部记录。
 * @note 需在 Wrapper 首次被调用前注册一次，重复注册会被 Wrapper 拒绝。
 *
 */
void bsp_adapter_xxx_register(void);
//************************* Function Prototypes end **************************//

#endif
//******************************* End of File ********************************//
