/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_chipflash_driver.h
 *
 * @par dependencies
 * - stm32f4xx.h
 * - main.h
 *
 * @author YA
 *
 * @brief STM32F411 片上 Flash 擦写接口（SPL 风格, 按扇区操作）。 *
 * Processing flow:
 *
 * call directly.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef __FLASH_H
#define __FLASH_H

//********************************Includes*********************************//

#include "stm32f4xx.h"
#include "main.h"

//********************************Includes*********************************//


/* STM32F411 Flash 扇区起始地址（容量不均匀, 低地址 16KB, 高地址 128KB。*/
#define ADDR_FLASH_SECTOR_0     ((u32)0x08000000) /* 扇区 0, 16  KB */
#define ADDR_FLASH_SECTOR_1     ((u32)0x08004000) /* 扇区 1, 16  KB */
#define ADDR_FLASH_SECTOR_2     ((u32)0x08008000) /* 扇区 2, 16  KB */
#define ADDR_FLASH_SECTOR_3     ((u32)0x0800C000) /* 扇区 3, 16  KB */
#define ADDR_FLASH_SECTOR_4     ((u32)0x08010000) /* 扇区 4, 64  KB */
#define ADDR_FLASH_SECTOR_5     ((u32)0x08020000) /* 扇区 5, 128 KB */
#define ADDR_FLASH_SECTOR_6     ((u32)0x08040000) /* 扇区 6, 128 KB */
#define ADDR_FLASH_SECTOR_7     ((u32)0x08060000) /* 扇区 7, 128 KB */
#define ADDR_FLASH_SECTOR_8     ((u32)0x08080000) /* 扇区 8, 128 KB */
#define ADDR_FLASH_SECTOR_9     ((u32)0x080A0000) /* 扇区 9, 128 KB */
#define ADDR_FLASH_SECTOR_10    ((u32)0x080C0000) /* 扇区 10, 128 KB */
#define ADDR_FLASH_SECTOR_11    ((u32)0x080E0000) /* 扇区 11, 128 KB */

/**
 * @brief 擦除 [addr, addr+size) 覆盖到的所有扇区。 *
 * @param[in] addr : 起始地址（内。Flash 空间）。 * @param[in] size : 擦除字节数（> 0）。 *
 * @return uint8_t : 0 成功, 1 失败。 */
uint8_t Flash_erase(u32 addr, u32 size);

/**
 * @brief 擦除指定扇区（SPL 封装, 解锁 -> 。-> 加锁）。 *
 * @param[in] FLASH_Sector : FLASH_Sector_x 宏之一。 *
 * @return FLASH_Status : SPL 状态码。 */
FLASH_Status EreaseAppSector(uint32_t FLASH_Sector);

/**
 * @brief 向指定地址写入 32 位字。 *
 * @param[in] address : 写入地址。 字节对齐）。 * @param[in] data    : 待写。32 位数据。 *
 * @return void。 */
void Flash_Write(uint32_t address, uint32_t data);

#endif /* __FLASH_H */
