/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file ota_flash_handler.c
 *
 * @par dependencies
 * - ota_flash_handler.h
 * - ota_config.h
 * - service_externflash_manage.h
 * - string.h
 *
 * @author YA
 *
 * @brief OTA 外部 Flash 顺序缓存实现。
 *
 * Processing flow:
 *
 * 接收数据 -> 子扇区缓存 -> 外部 Flash 写入 -> 按顺序读取镜像。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "ota_flash_handler.h"

#include "ota_config.h"
#include "service_externflash_manage.h"
#include <string.h>


typedef struct
{
    u8 data[SUBSECTOR_SIZE]; /* 当前子扇区缓存。这里应该使用ABbuffer或者内存池 */
    u32 write_index;         /* 已接收数据总长度。 */
    u32 read_index;          /* 已读取数据总长度。 */
} ota_flash_context_t;


static ota_flash_context_t s_context;

/********************************* Function declarations *********************/
static u8 ota_flash_has_enough_capacity(u32 append_length);
/********************************* Function declarations *********************/


/**
 * @brief 检查追加数据是否仍在 OTA 容量范围内。
 *
 * @param[in] append_length : 待追加数据长度，单位为字节。
 * @return u8 : 1 表示容量足够，0 表示容量不足或参数无效。
 */
static u8 ota_flash_has_enough_capacity(u32 append_length)
{
    if ((0U == append_length) ||
        (s_context.write_index > OTA_APP_MAX_SIZE) ||
        (append_length > (OTA_APP_MAX_SIZE - s_context.write_index)))
    {
        return 0U;
    }

    return 1U;
}


/**
 * @brief 清零读写索引和ram模拟扇区缓存。
 *
 * @return void : 无返回值。
 */
void ota_flash_init(void)
{
    /* 新一轮下载从空缓存和零索引开始。 */
    (void)memset(&s_context, 0, sizeof(s_context));
}


/**
 * @brief 擦除 OTA A 区，供一次新的下载开始时调用。
 *
 * @return u8 : 0 表示成功，1 表示擦除失败。
 */
u8 ota_flash_erase(void)
{
    /* 擦除外部 Flash 的 OTA A 区。 */
    return (extern_flash_erase(OTA_AREA_A_ADDRESS,
                               OTA_AREA_A_SIZE) == Ext_Flash_OK) ?
           0U : 1U;
}


/**
 * @brief 追加数据，缓存满一个子扇区后提交到外部 Flash 服务。
 *
 * @param[in] data : 待写入的数据。
 * @param[in] length : 待写入数据长度，单位为字节。
 * @return u8 : 0 表示成功，1 表示参数或写入失败。
 */
u8 ota_flash_write_data(u8 *data, u32 length)
{
    u32 index;

    /* 检查指针和 OTA 容量，避免缓存或地址越界。 */
    if ((NULL == data) || (0U == length) ||
        (0U == ota_flash_has_enough_capacity(length)))
    {
        return 1U;
    }

    for (index = 0U; index < length; ++index)
    {
        /* 按当前位置写入子扇区缓存。 */
        s_context.data[s_context.write_index % SUBSECTOR_SIZE] = data[index];
        ++s_context.write_index;

        /* 缓存填满后，提交当前完整子扇区。 */
        if ((s_context.write_index % SUBSECTOR_SIZE) == 0U)
        {
            if (extern_flash_write(OTA_AREA_A_ADDRESS +
                                   s_context.write_index - SUBSECTOR_SIZE,
                                   SUBSECTOR_SIZE,
                                   s_context.data) != Ext_Flash_OK)
            {
                /* 外部 Flash 写入失败，停止本次提交。 */
                return 1U;
            }
        }
    }

    return 0U;
}


/**
 * @brief 提交最后一个未满子扇区。
 *
 * @return u8 : 0 表示成功，1 表示写入失败。
 */
u8 ota_flash_write_data_end(void)
{
    /* 计算最后一个未满子扇区的有效数据长度。 */
    u32 remain = s_context.write_index % SUBSECTOR_SIZE;

    /* 数据刚好对齐时，没有尾部缓存需要写入。 */
    if (0U == remain)
    {
        return 0U;
    }

    /* 将尾部有效数据写入 A 区对应地址。 */
    return (extern_flash_write(OTA_AREA_A_ADDRESS +
                               s_context.write_index - remain,
                               remain,
                               s_context.data) == Ext_Flash_OK) ?
           0U : 1U;
}


/**
 * @brief 从已写入的 OTA 区域读取一个数据块。
 *
 * @param[out] data : 读取数据的缓冲区。
 * @param[out] length : 返回实际读取长度，单位为字节。
 * @return u8 : 0 表示成功，1 表示没有数据，2 表示参数或读取失败。
 */
u8 ota_flash_read_data(u8 *data, u16 *length)
{
    u32 remain;
    u32 read_size;

    /* 读取缓冲区和长度输出指针必须有效。 */
    if ((NULL == data) || (NULL == length))
    {
        return 2U;
    }

    /* 只统计已经写入但尚未读取的数据。 */
    remain = (s_context.write_index > s_context.read_index) ?
             (s_context.write_index - s_context.read_index) : 0U;

    /* 写入索引追上读取索引，表示镜像已经读完。 */
    if (0U == remain)
    {
        return 1U;
    }

    /* 单次最多读取一个子扇区，避免超过调用方预期。 */
    read_size = (remain > SUBSECTOR_SIZE) ? SUBSECTOR_SIZE : remain;
    if (extern_flash_read(OTA_AREA_A_ADDRESS + s_context.read_index,
                          read_size,
                          data) != Ext_Flash_OK)
    {
        /* 外部 Flash 读取失败，保留当前读取索引。 */
        return 2U;
    }

    /* 读取成功后推进索引，并返回实际长度。 */
    *length = (u16)read_size;
    s_context.read_index += read_size;
    return 0U;
}
