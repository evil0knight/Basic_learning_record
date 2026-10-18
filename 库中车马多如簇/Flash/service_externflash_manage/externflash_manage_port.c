/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file externflash_manage_port.c
 *
 * @par dependencies
 * - externflash_manage_port.h
 * - bsp_wrapper_flash.h
 *
 * @author YA
 *
 * @brief 上层 Service 通过本文件调用 bsp_wrapper_flash
 *
 * Processing flow:
 * 这里完全就是调用wrapper层提供上来的,这里是业务层的一层解耦,flash人员只看这里的函数
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes*********************************//

#include "externflash_manage_port.h"
#include "bsp_wrapper_flash.h"

//********************************Includes*********************************//


/**
 * @brief 初始化外部 Flash（转发）。
 */
void externflash_init(void)
{
    bsp_wrapper_flash_init();
}

/**
 * @brief 去初始化外部 Flash（转发）。
 */
void externflash_deinit(void)
{
    bsp_wrapper_flash_deinit();
}

/**
 * @brief 读外部 Flash（转发）。
 */
void externflash_read(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    bsp_wrapper_flash_read(addr, size, p_buf);
}

/**
 * @brief 写外部 Flash（先擦后写, 转发）。
 */
void externflash_write(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    bsp_wrapper_flash_write(addr, size, p_buf);
}

/**
 * @brief 写外部 Flash（不擦除, 转发）。
 */
void externflash_write_noerase(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    bsp_wrapper_flash_write_noerase(addr, size, p_buf);
}

/**
 * @brief 整片擦除外部 Flash（转发）。
 */
void externflash_erasechip(void)
{
    bsp_wrapper_flash_erasechip();
}

/**
 * @brief 擦除外部 Flash 单个扇区（转发）。
 */
void externflash_erasesector(uint32_t addr)
{
    bsp_wrapper_flash_erasesector(addr);
}

/**
 * @brief 唤醒外部 Flash（转发）。
 */
void externflash_wakeup(void)
{
    bsp_wrapper_flash_wakeup();
}

/**
 * @brief 休眠外部 Flash（转发）。
 */
void externflash_sleep(void)
{
    bsp_wrapper_flash_sleep();
}

/**
 * @brief 外部 Flash 自检（转发）。
 */
void externflash_test(void)
{
    bsp_wrapper_flash_test();
}
