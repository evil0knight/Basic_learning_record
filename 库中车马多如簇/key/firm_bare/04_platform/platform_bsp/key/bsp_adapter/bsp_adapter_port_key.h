/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_adapter_port_key.h
 * @par dependencies
 * - ../../../../05_bsp/bsp_key/bsp_key_types.h
 * @author YA
 * @brief 组装 {pf_init, pf_control, p_context} 并注册到 Wrapper。
 * Processing flow:
 * 组装 ops -> Wrapper 注册 -> Wrapper init 触发 Handler 挂载。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef BSP_ADAPTER_PORT_KEY_H
#define BSP_ADAPTER_PORT_KEY_H

#include "../../../../05_bsp/bsp_key/bsp_key_types.h"

/**
 * @brief 把 Adapter 的 {pf_init, pf_control, p_context} 注册到 Wrapper。
 * @param[in] 无
 * @return en_key_status_t 注册结果；实际初始化由 bsp_wrapper_key_init 触发。
 */
en_key_status_t drv_adapter_key_register(void);

#endif /* BSP_ADAPTER_PORT_KEY_H */
