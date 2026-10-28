/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_adapter_port_key.h
 * @par dependencies
 * - ../../../05_bsp/bsp_key/bsp_key_types.h
 * @author YA
 * @brief 组装按键设备描述并挂载到 Wrapper。
 * Processing flow:
 * 平台接口及输入参数 -> 设备描述 -> Wrapper 注册。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef BSP_ADAPTER_PORT_KEY_H
#define BSP_ADAPTER_PORT_KEY_H

#include "../../../05_bsp/bsp_key/bsp_key_types.h"

/**
 * @brief 创建按键设备描述并注册到 Wrapper 的 0 号槽位。
 * @param[in] 无；平台接口和设备操作由 Adapter 静态组装。
 * @return en_key_status_t 接口绑定及注册结果；GPIO 和时基须预先就绪。
 */
en_key_status_t drv_adapter_key_register(void);

#endif /* BSP_ADAPTER_PORT_KEY_H */
