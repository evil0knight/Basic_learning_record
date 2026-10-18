/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file service_externflash_manage.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief 外部 Flash 异步读写服务接口（通用绝对地址, 不绑定业务分区）。
 *
 * Processing flow:
 *
 * 上层调 extern_flash_read/write/erase (绝对地址)
 *   -> 发事件到事件组
 *   -> storage_manager_task 单任务串行处理
 *   -> 经 port 转到底层 Flash 驱动
 *   -> 完成通过事件组同步。
 *
 * 业务层（OTA 区 / LVGL 区等）自行定义地址范围后调用本服务。
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef SERVICE_EXTERNFLASH_MANAGE_H
#define SERVICE_EXTERNFLASH_MANAGE_H

//********************************Includes*********************************//

#include <stdint.h>

//********************************Includes*********************************//


/* 通用请求类型事件位 */
#define EVENT_READ                      (1U << 0)
#define EVENT_WRITE                     (1U << 1)
#define EVENT_ERASE                     (1U << 2)

/* 服务层状态返回码 */
typedef enum
{
    Ext_Flash_OK               = 0,
    Ext_Flash_ERROR            = 1,
    Ext_Flash_ERRORTIMEOUT     = 2,
    Ext_Flash_ERRORRESOURCE    = 3,
    Ext_Flash_ERRORPARAMETER   = 4,
    Ext_Flash_ERRORNOMEMORY    = 5,
    Ext_Flash_RESERVED         = 0x7FFFFFFF
} ext_flash_status_t;

/**
 * @brief 同步读外部 Flash（绝对地址）。
 *
 * @param[in]  addr : 读起始地址。
 * @param[in]  size : 读取长度（> 0）。
 * @param[out] buf  : 读缓冲（非空）。
 *
 * @return ext_flash_status_t : 状态码。
 */
ext_flash_status_t extern_flash_read(uint32_t addr, uint32_t size,
                                     uint8_t *buf);

/**
 * @brief 同步写外部 Flash（绝对地址, 内部先擦后写）。
 *
 * @param[in] addr : 写起始地址。
 * @param[in] size : 写入长度（> 0）。
 * @param[in] buf  : 待写数据（非空）。
 *
 * @return ext_flash_status_t : 状态码。
 */
ext_flash_status_t extern_flash_write(uint32_t addr, uint32_t size,
                                      uint8_t *buf);

/**
 * @brief 同步擦除外部 Flash 一段区域（按 4KB 对齐循环擦除扇区）。
 *
 * @param[in] addr : 起始地址。
 * @param[in] size : 擦除字节数。
 *
 * @return ext_flash_status_t : 状态码。
 */
ext_flash_status_t extern_flash_erase(uint32_t addr, uint32_t size);

/**
 * @brief 外部 Flash 存储管理任务（单任务串行处理事件组事件）。
 *
 * @param[in] argument : 任务参数（未使用）。
 *
 * @return void。
 */
void storage_manager_task(void *argument);

#endif /* SERVICE_EXTERNFLASH_MANAGE_H */
