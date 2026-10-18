/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file ota_flash_handler.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief OTA 外部 Flash 顺序缓存接口。
 *
 * Processing flow:
 *
 * 初始化缓存 -> 写入镜像 -> 提交尾部数据 -> 顺序读取镜像。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef OTA_FLASH_HANDLER_H
#define OTA_FLASH_HANDLER_H

//******************************** Includes *********************************//
#include <stdint.h>
//******************************** Includes *********************************//

//******************************** Types ************************************//
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
//******************************** Types ************************************//

//******************************** Definitions ******************************//
#define EXTERN_Flash   (1U)
#define SUBSECTOR_SIZE (4096U)
#define PAGE_SIZE      (256U)
//******************************** Definitions ******************************//

//******************************** Declaring ********************************//
/**
 * @brief 清零 OTA 外部 Flash 缓存上下文。
 * @return void : 无返回值。
 */
void ota_flash_init(void);

/**
 * @brief 擦除 OTA A 区。
 * @return u8 : 0 表示成功，1 表示失败。
 */
u8 ota_flash_erase(void);

/**
 * @brief 追加 OTA 数据并写入完整子扇区。
 * @param[in] data : 待写入的数据。
 * @param[in] length : 数据长度，单位为字节。
 * @return u8 : 0 表示成功，1 表示失败。
 */
u8 ota_flash_write_data(u8 *data, u32 length);

/**
 * @brief 写入最后一个未满子扇区。
 * @return u8 : 0 表示成功，1 表示失败。
 */
u8 ota_flash_write_data_end(void);

/**
 * @brief 读取 OTA 区域中的下一个数据块。
 * @param[out] data : 读取数据的缓冲区。
 * @param[out] length : 返回实际读取长度。
 * @return u8 : 0 表示成功，1 表示没有数据，2 表示失败。
 */
u8 ota_flash_read_data(u8 *data, u16 *length);
//******************************** Declaring ********************************//

#endif /* OTA_FLASH_HANDLER_H */
