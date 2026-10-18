/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_adapter_port_flash.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief 外部 Flash 平台适配层接口。
 *
 * Processing flow:
 *
 * 平台初始化时调用 bsp_adapter_flash_register，完成接口挂载和驱动注册。
 *
 * @version V1.0 2026-10-17
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef BSP_ADAPTER_PORT_FLASH_H
#define BSP_ADAPTER_PORT_FLASH_H

#include <stdint.h>

/**
 * @brief 注册外部 Flash Adapter。
 *
 * Steps:
 *  1. 准备平台 SPI、时基和 OS 接口。
 *  2. 准备 Handler 事件转发函数。
 *  3. 将函数表注册到 bsp_wrapper_flash。
 *
 * @return void : 无返回值。
 */
void bsp_adapter_flash_register(void);

#endif /* BSP_ADAPTER_PORT_FLASH_H */
