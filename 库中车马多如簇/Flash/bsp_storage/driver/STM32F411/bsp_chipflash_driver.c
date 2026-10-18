/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_chipflash_driver.c
 *
 * @par dependencies
 * - bsp_chipflash_driver.h
 *
 * @author YA（代码移植）
 *
 * @brief STM32F411 片上 Flash 擦写实现（SPL, 按扇区操作）。 *
 * Processing flow:
 *
 * Flash_erase: 地址 -> 扇区。-> 循环 EreaseAppSector。 * Flash_Write: 解锁 -> ProgramWord -> 加锁。 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes*********************************//

#include "Flash.h"

//********************************Includes*********************************//


/* Private function prototypes -----------------------------------------------*/
static uint16_t STMFLASH_GetFlashSector(u32 addr);

/* 索引-扇区号映射表。..11。*/
const uint32_t Flash_Sectorsize[12] =
{
    FLASH_Sector_0,
    FLASH_Sector_1,
    FLASH_Sector_2,
    FLASH_Sector_3,
    FLASH_Sector_4,
    FLASH_Sector_5,
    FLASH_Sector_6,
    FLASH_Sector_7,
    FLASH_Sector_8,
    FLASH_Sector_9,
    FLASH_Sector_10,
    FLASH_Sector_11
};

/**
 * @brief 擦除 [addr, addr+size) 覆盖到的所有扇区。 *
 * Steps:
 *  1. 参数检查。 size 。0 直接返回失败。 *  2. 用地址求首尾扇区号。 *  3. 遍历映射。 落在区间内的扇区依次调用 EreaseAppSector。 *
 * @param[in] addr : 起始地址。 * @param[in] size : 擦除字节数。 *
 * @return uint8_t : 0 成功, 1 失败。 */
uint8_t Flash_erase(u32 addr, u32 size)
{
    uint32_t flash_start_sector = 0;
    uint32_t flash_end_sector   = 0;

    /* 参数检查。 size=0 不执行。*/
    if (0U == size)
    {
       return 1U;
    }

    /* 求起止扇区号（含端点。*/
    flash_start_sector = STMFLASH_GetFlashSector(addr);
    flash_end_sector   = STMFLASH_GetFlashSector(addr + size - 1u);

    /* 遍历扇区。 擦除区间内的所有扇。*/
    for (uint8_t i = 0; i < 12; i++)
    {
        if (((Flash_Sectorsize[i]) >= flash_start_sector) &&
            ((Flash_Sectorsize[i]) <= flash_end_sector))
        {
            if (FLASH_COMPLETE != EreaseAppSector(Flash_Sectorsize[i]))
            {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * @brief 通过地址定位到所在的 F411 Flash 扇区号。 *
 * @param[in] addr : Flash 空间地址。 *
 * @return uint16_t : FLASH_Sector_x 之一。 */
static uint16_t STMFLASH_GetFlashSector(u32 addr)
{
    /* 。4 扇区。16KB, 。4 扇区 64KB, 其余。128KB */
    if (addr < ADDR_FLASH_SECTOR_1)       return FLASH_Sector_0;
    else if (addr < ADDR_FLASH_SECTOR_2)  return FLASH_Sector_1;
    else if (addr < ADDR_FLASH_SECTOR_3)  return FLASH_Sector_2;
    else if (addr < ADDR_FLASH_SECTOR_4)  return FLASH_Sector_3;
    else if (addr < ADDR_FLASH_SECTOR_5)  return FLASH_Sector_4;
    else if (addr < ADDR_FLASH_SECTOR_6)  return FLASH_Sector_5;
    else if (addr < ADDR_FLASH_SECTOR_7)  return FLASH_Sector_6;
    else if (addr < ADDR_FLASH_SECTOR_8)  return FLASH_Sector_7;
    else if (addr < ADDR_FLASH_SECTOR_9)  return FLASH_Sector_8;
    else if (addr < ADDR_FLASH_SECTOR_10) return FLASH_Sector_9;
    else if (addr < ADDR_FLASH_SECTOR_11) return FLASH_Sector_10;
    return FLASH_Sector_11;
}

/**
 * @brief 解锁 Flash, 等忙位清除。 */
void Flash_Unlock(void)
{
    FLASH_Unlock();

    /* 。Flash 控制器空。 保证解锁完成 */
    while (FLASH_BUSY == FLASH_GetStatus())
    {
    }
}

/**
 * @brief 加锁 Flash。 */
void Flash_Lock(void)
{
    FLASH_Lock();
}

/**
 * @brief 擦除指定扇区（SPL 封装）。 *
 * @param[in] FLASH_Sector : FLASH_Sector_x 之一。 *
 * @return FLASH_Status : SPL 状态码。 */
FLASH_Status EreaseAppSector(uint32_t FLASH_Sector)
{
    FLASH_Status FLASHStatus;

    /* 解锁 -> 擦除 -> 加锁 */
    Flash_Unlock();
    FLASHStatus = FLASH_EraseSector(FLASH_Sector, VoltageRange_3);
    Flash_Lock();

    return FLASHStatus;
}

/**
 * @brief 向指定地址写入 32 位字。 *
 * Steps:
 *  1. 解锁 Flash。 *  2. 。FLASH_ProgramWord 编程 32 位。 *  3. 根据返回状态做分支（成。/ 失败日志预留）。 *  4. 加锁 Flash。 *
 * @param[in] address : 写入地址。 字节对齐）。 * @param[in] data    : 待写。32 位数据。 *
 * @return void。 */
void Flash_Write(uint32_t address, uint32_t data)
{
    FLASH_Status status;

    /* 解锁 Flash */
    Flash_Unlock();

    /* 编程一。32 位字 */
    status = FLASH_ProgramWord(address, data);

    /* 结果分支: 预留日志钩子 */
    if (FLASH_COMPLETE == status)
    {
        /* log_d("Flash_Write success"); */
    }
    else
    {
        /* log_e("Flash_Write fail"); */
    }

    /* 加锁 Flash */
    Flash_Lock();
}
