/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_w25q64_driver.h
 *
 * @par dependencies
 * - stdint.h
 * - bsp_w25q64_reg.h
 *
 * @author YA
 *
 * @brief W25Q64 驱动层接口（函数指针注入 SPI / 时基 / 延时, 不耦合 OS）。 *
 * Processing flow:
 *
 * Adapter 提供 spi_w25q64_interface_t / w25qxx_timebase_interface_t /
 * w25qxx_os_delay_interface_t 三个接口 -> 调用 w25q64_inst 实例化驱动 ->
 * 通过驱动实例化。pf_* 函数操作芯片。 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef BSP_W25Q64_DRIVER_H
#define BSP_W25Q64_DRIVER_H

//********************************Includes*********************************//

#include "bsp_w25q64_reg.h"

#include <stdint.h>

//********************************Includes*********************************//


/* 驱动恒用 OS 抽象 + 硬件 SPI */

/* W25Q64 状态返回码 */
typedef enum
{
    W25Q64_OK = 0,
    W25Q64_ERROR,
    W25Q64_ERROR_TIMEOUT,
    W25Q64_ERROR_RESOURCE,
    W25Q64_ERROR_PARAMETER,
    W25Q64_ERROR_NOMEM,
    W25Q64_BUSY,
    W25Q64_SLEEP,
    W25Q64_WAKEUP,
    W25Q64_ERROR_ISR,
    W25Q64_ERROR_UNKNOWN,
    W25Q64_RESERVED = 0x1FFFFFFF
} w25q64_status_t;

/* ===== SPI 逐字节接口（Adapter 。core_spi 适配。==== */
typedef struct
{
    void    (*pf_spi_init)(void *);         /* SPI 初始化。*/
    void    (*pf_spi_deinit)(void *);       /* SPI 去初始化 */
    uint8_t (*pf_spi_cs_set)(void);         /* 片选拉。*/
    uint8_t (*pf_spi_cs_deset)(void);       /* 片选拉。*/
    uint8_t (*pf_spi_write_byte)(uint8_t);  /* 。1 字节 */
    uint8_t (*pf_spi_read_byte)(void);      /* 。1 字节 */

    /* OS 临界区（保证 CS 期间数据完整。*/
    void    (*pf_critical_enter)(void);
    void    (*pf_critical_exit)(void);
} spi_w25q64_interface_t;

/* ===== 时基接口 ===== */
typedef struct
{
    uint32_t (*pf_tick_count_get)(void);    /* 取当。tick */
} w25qxx_timebase_interface_t;

/* ===== OS 延时接口 ===== */
typedef struct
{
    void (*pf_os_delay_ms)(const uint32_t); /* 阻塞延时 ms */
} w25qxx_os_delay_interface_t;

/* 驱动私有数据前向声明 */
typedef struct w25q64_private_data w25q64_private_data_t;

/* ===== W25Q64 驱动实例 ===== */
typedef struct
{
    /* 注入接口 */
    spi_w25q64_interface_t       *p_spi_w25q64_instance;
    w25qxx_timebase_interface_t  *p_timebase_instance;
    w25qxx_os_delay_interface_t  *p_os_delay_instance;
    w25q64_private_data_t        *p_private_data;

    /* 实例函数指针（实例化时挂载） */
    w25q64_status_t (*pf_inst)(void *const p_w25q64_instance,
                               spi_w25q64_interface_t *const p_spi_w25q64_instance,
                               w25qxx_timebase_interface_t *const p_timebase_instance,
                               w25qxx_os_delay_interface_t *const p_os_delay_instance);

    w25q64_status_t (*pf_init)(void *const);
    w25q64_status_t (*pf_deinit)(void *const);
    w25q64_status_t (*pf_read_id)(void *const, uint16_t *const);
    w25q64_status_t (*pf_read)(void *const, uint8_t *, uint32_t, uint32_t);
    w25q64_status_t (*pf_write_erase)(void *const, const uint8_t *,
                                      uint32_t, uint32_t);
    w25q64_status_t (*pf_write_no_erase)(void *const, const uint8_t *,
                                         uint32_t, uint32_t);
    w25q64_status_t (*pf_erase_chip)(void *const);
    w25q64_status_t (*pf_erase_sector)(void *const, uint32_t);
    w25q64_status_t (*pf_sleep)(void *const);
    w25q64_status_t (*pf_wakeup)(void *const);
} bsp_w25q64_driver_t;

/**
 * @brief 实例化 W25Q64 驱动。 *
 * Steps:
 *  1. 分配私有数据并清零。 *  2. 参数检查。 实例 / SPI / 时基 / 延时接口均非空。 *  3. 挂载接口 + 函数指针。 *  4. 调驱动内部 init（SPI 初始化。 。ID 校验）。 *
 * @param[in] p_w25q64_instance       : 驱动实例指针。 * @param[in] p_spi_w25q64_instance   : SPI 接口。 * @param[in] p_timebase_instance     : 时基接口。 * @param[in] p_os_delay_instance     : 延时接口。 *
 * @return w25q64_status_t : 状态码。 */
w25q64_status_t w25q64_inst(
    bsp_w25q64_driver_t         *const p_w25q64_instance,
    spi_w25q64_interface_t      *const p_spi_w25q64_instance,
    w25qxx_timebase_interface_t *const p_timebase_instance,
    w25qxx_os_delay_interface_t *const p_os_delay_instance);

/**
 * @brief 读取 W25Q64 。Manufacturer+Device ID。 *
 * @param[in]  p_w25q64_instance : 驱动实例化。 * @param[out] p_id              : 返回 ID。 *
 * @return w25q64_status_t : 状态码。 */
w25q64_status_t read_w25qxx_id(bsp_w25q64_driver_t *const p_w25q64_instance,
                               uint16_t *const p_id);

/**
 * @brief 读取 W25Q64 当前睡眠状态。 *
 * @param[in] p_w25q64_instance : 驱动实例化。 *
 * @return w25q64_status_t : W25Q64_SLEEP / W25Q64_WAKEUP 等。 */
w25q64_status_t read_w25qxx_isSleep(bsp_w25q64_driver_t *const p_w25q64_instance);

#endif /* BSP_W25Q64_DRIVER_H */
