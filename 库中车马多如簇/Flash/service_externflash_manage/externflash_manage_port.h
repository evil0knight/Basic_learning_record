/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file externflash_manage_port.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief 外部 Flash 平台 port 接口: 把分区服务请求转发给 bsp_wrapper_flash。
 *
 * Processing flow:
 *
 * 本层不包含器件 / OS 类型, 只做接口转发, 保证 service 与 BSP 解耦。
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef EXTERNFLASH_MANAGE_PORT_H
#define EXTERNFLASH_MANAGE_PORT_H

//********************************Includes*********************************//

#include <stdint.h>

//********************************Includes*********************************//


/**
 * @brief 初始化外部 Flash（转发到 wrapper）。
 */
void externflash_init(void);

/**
 * @brief 去初始化外部 Flash（转发到 wrapper）。
 */
void externflash_deinit(void);

/**
 * @brief 读外部 Flash（转发到 wrapper）。
 *
 * @param[in]  addr  : 读起始地址。
 * @param[in]  size  : 读取长度。
 * @param[out] p_buf : 读缓冲。
 */
void externflash_read(uint32_t addr, uint32_t size, uint8_t *p_buf);

/**
 * @brief 写外部 Flash（先擦后写, 转发到 wrapper）。
 */
void externflash_write(uint32_t addr, uint32_t size, uint8_t *p_buf);

/**
 * @brief 写外部 Flash（不擦除, 转发到 wrapper）。
 */
void externflash_write_noerase(uint32_t addr, uint32_t size, uint8_t *p_buf);

/**
 * @brief 擦除外部 Flash 一个扇区（转发到 wrapper）。
 *
 * @param[in] addr : 扇区起始地址。
 */
void externflash_erasesector(uint32_t addr);

/**
 * @brief 唤醒外部 Flash（转发到 wrapper）。
 */
void externflash_wakeup(void);

/**
 * @brief 休眠外部 Flash（转发到 wrapper）。
 */
void externflash_sleep(void);

/**
 * @brief 外部 Flash 自检（转发到 wrapper）。
 */
void externflash_test(void);

/**
 * @brief 整片擦除外部 Flash（转发到 wrapper）。
 */
void externflash_erasechip(void);

#endif /* EXTERNFLASH_MANAGE_PORT_H */
