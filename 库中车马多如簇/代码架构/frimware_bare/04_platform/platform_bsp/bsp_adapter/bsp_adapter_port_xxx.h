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
 * @brief 将 XXX BSP Handler 挂载到平台 Wrapper。
 *
 * Processing flow:
 *
 * 组装 Port 接口 -> 初始化 Handler -> 注册 Wrapper 操作接口。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef FRIMWARE_BARE_BSP_ADAPTER_PORT_XXX_H
#define FRIMWARE_BARE_BSP_ADAPTER_PORT_XXX_H

#include <stdint.h>

/**
 * @brief 注册 XXX Adapter。
 * @return void。
 */
void bsp_adapter_xxx_register(void);

#endif /* FRIMWARE_BARE_BSP_ADAPTER_PORT_XXX_H */
