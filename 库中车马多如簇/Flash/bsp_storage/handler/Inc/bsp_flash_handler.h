/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_flash_handler.h
 *
 * @par dependencies
 * - stdint.h
 * - stddef.h
 * - bsp_w25q64_driver.h
 *
 * @author YA
 *
 * @brief W25Q64 Flash Handler 接口（事件驱动 + 单任务串行, 天然互斥）。
 *
 * Processing flow:
 *
 * 上层 flash_handler_event_put 投事件入队 ->
 * flash_handler_thread 依次 queue_receive ->
 * flash_event_handle 分发到驱动的 pf_* 函数 ->
 * 事件携带 pf_callback 时回调通知完成。
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef BSP_FLASH_HANDLER_H
#define BSP_FLASH_HANDLER_H

//********************************Includes*********************************//

#include "bsp_w25q64_driver.h"

#include <stdint.h>
#include <stddef.h>

//********************************Includes*********************************//


/* Handler 层状态返回码 */
typedef enum
{
    FLASH_HANDLER_OK = 0,
    FLASH_HANDLER_ERROR,
    FLASH_HANDLER_ERROR_TIMEOUT,
    FLASH_HANDLER_ERROR_RESOURCE,
    FLASH_HANDLER_ERROR_PARAMETER,
    FLASH_HANDLER_ERROR_NOMEM,
    FLASH_HANDLER_ERROR_ISR,
    FLASH_HANDLER_ERROR_UNKNOWN,
    FLASH_HANDLER_ERROR_NOTINIT,
    FLASH_HANDLER_RESERVED = 0x1FFFFFFF
} flash_handler_status_t;

/* Flash 事件类型 */
typedef enum
{
    FLASH_HANDLER_EVENT_READ = 0,
    FLASH_HANDLER_EVENT_WRITE,
    FLASH_HANDLER_EVENT_WRITE_NOERASE,
    FLASH_HANDLER_EVENT_ERASE_CHIP,
    FLASH_HANDLER_EVENT_ERASE_SECTOR,
    FLASH_HANDLER_EVENT_WAKEUP,
    FLASH_HANDLER_EVENT_SLEEP,
    FLASH_HANDLER_EVENT_TEST
} flash_handler_event_type_t;

/* Flash 事件（READ/WRITE 需要地址 + 长度 + 缓冲, 其他可传 NULL） */
typedef struct
{
    uint32_t                    addr;        /* 读/写地址 */
    uint32_t                    size;        /* 读/写长度 */
    uint8_t                    *p_data;      /* 读/写缓冲 */
    flash_handler_event_type_t  type;        /* 事件类型 */
    void (*pf_callback)(void *);             /* 完成回调, 可为 NULL */
} flash_handler_event_t;

/* ===== OS 接口（函数指针注入）===== */
typedef struct
{
    void (*pf_os_delay)(uint32_t);
    flash_handler_status_t (*pf_os_queue_create)(uint32_t item_num,
                                                  uint32_t item_size,
                                                  void **queue_handle);
    flash_handler_status_t (*pf_os_queue_send)(void *queue_handle,
                                                void *item,
                                                uint32_t timeout);
    flash_handler_status_t (*pf_os_queue_receive)(void *queue_handle,
                                                   void *msg,
                                                   uint32_t timeout);
} flash_handler_os_interface_t;

/* 实例化入参: 实例化时注入的接口包 */
typedef struct
{
    spi_w25q64_interface_t       *p_spi_w25q64_instance;   /* SPI 接口 */
    w25qxx_timebase_interface_t  *p_timebase_instance;     /* 时基接口 */
    flash_handler_os_interface_t *p_os_instance;           /* OS 接口 */
    w25qxx_os_delay_interface_t  *p_os_delay_instance;     /* 延时接口 */
} flash_handler_all_input_arg_t;

/* Handler 私有数据前向声明 */
typedef struct flash_handler_private_data flash_handler_private_data_t;

/* ===== Handler 实例 ===== */
typedef struct
{
    /* 注入的接口 */
    spi_w25q64_interface_t       *p_spi_w25q64_instance;
    w25qxx_timebase_interface_t  *p_timebase_instance;
    flash_handler_os_interface_t *p_os_instance;
    w25qxx_os_delay_interface_t  *p_os_delay_instance;

    /* 驱动实例 */
    bsp_w25q64_driver_t          *p_w25q64_instance;

    /* 事件队列句柄 */
    void                         *p_event_queue_handle;

    /* 私有数据 */
    flash_handler_private_data_t *p_private_data;
} bsp_flash_handler_t;

/**
 * @brief 实例化 Flash Handler（注入接口 + 创建事件队列 + 初始化驱动）。
 *
 * @param[in] p_flash_handler_instance : Handler 实例指针。
 * @param[in] input_arg                : 接口包。
 *
 * @return flash_handler_status_t : 状态码。
 */
flash_handler_status_t bsp_flash_handler_inst(
    bsp_flash_handler_t           *p_flash_handler_instance,
    flash_handler_all_input_arg_t *input_arg);

/**
 * @brief 处理一个 Flash 事件（分发到驱动的 pf_* 函数）。
 *
 * @param[in] p_flash_handler_instance : Handler 实例化。
 * @param[in] event                    : 事件内容。
 *
 * @return flash_handler_status_t : 状态码。
 */
flash_handler_status_t flash_event_handle(
    const bsp_flash_handler_t    *const p_flash_handler_instance,
    const flash_handler_event_t  *const event);

/**
 * @brief 投递事件到事件队列（上层异步调用入口）。
 *
 * @param[in] event : 事件内容。
 *
 * @return flash_handler_status_t : 状态码。
 */
flash_handler_status_t flash_handler_event_put(
    const flash_handler_event_t *const event);

/**
 * @brief Handler 任务: 从队列取事件并串行处理, 完成时触发回调。
 *
 * @param[in] argument : flash_handler_all_input_arg_t 指针。
 *
 * @return void。
 */
void flash_handler_thread(void *argument);

#endif /* BSP_FLASH_HANDLER_H */
