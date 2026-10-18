/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_w25q64_driver.c
 *
 * @par dependencies
 * - bsp_w25q64_driver.h
 * - stdlib.h
 * - string.h
 *
 * @author YA (ported from the watch project).
 *
 * @brief W25Q64 Flash driver implementation.
 *
 * Processing flow:
 *
 * Wait for BUSY clear -> enter critical section -> assert CS ->
 * send command -> deassert CS -> exit critical section.
 * @version V1.0 2026-10-16
 *
 *****************************************************************************/

//********************************Includes*********************************//

#include "bsp_w25q64_driver.h"

#include <stdlib.h>
#include <string.h>

//********************************Includes*********************************//


#ifndef W25Q64_LOG
#define W25Q64_LOG(...) ((void)0)
#endif

#define W25Q64_INITED   (1U)
#define W25Q64_UNINITED (0U)

/* Initialization state check for current driver instance */
#define W25Q64_ISINITED \
    (W25Q64_INITED == (p_w25q64_instance->p_private_data->w25q64_inited))

/* W25Q64 driver private data */
typedef struct w25q64_private_data
{
    uint8_t         w25q64_inited;    /* Initialization flag */
    uint16_t        w25qxx_id;        /* Device ID */
    w25q64_status_t w25q64_isSleep;   /* Sleep state */
} w25q64_private_data_t;

/**
 * @brief 读取状态寄存器并返回 BUSY 状态。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @return uint8_t : 0 表示空闲，6 表示忙。
 */
static uint8_t __read_status(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    uint8_t status = 0;

    /* Protect the complete CS transaction */
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_READ_REG);
    status = p_w25q64_instance->p_spi_w25q64_instance->pf_spi_read_byte();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    /* BUSY bit set: return 6 as busy flag */
    return (1 == (status & 0x01)) ? 6U : 0U;
}

/**
 * @brief 读取 W25Q64 芯片 ID 并校验。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @return w25q64_status_t : ID 校验结果。
 */
static w25q64_status_t __w25q64read_id(
    bsp_w25q64_driver_t *const p_w25q64_instance)
{
    uint16_t id = 0;

    /* Protect the complete READ_ID transaction */
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_READ_ID);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(0x00);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(0x00);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(0x00);
    id  = p_w25q64_instance->p_spi_w25q64_instance->pf_spi_read_byte();
    id <<= 8;
    id |= p_w25q64_instance->p_spi_w25q64_instance->pf_spi_read_byte();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    /* Expected W25Q64 ID: 0xEF16 */
    if (0xEF16 == id)
    {
        p_w25q64_instance->p_private_data->w25qxx_id = id;
        return W25Q64_OK;
    }
    W25Q64_LOG("Read W25Q64 ID failed\n");
    return W25Q64_ERROR_RESOURCE;
}

/**
 * @brief 初始化 W25Q64 驱动和 SPI 接口。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @return w25q64_status_t : 初始化状态码。
 */
static w25q64_status_t __w25q64init(
    bsp_w25q64_driver_t *const p_w25q64_instance)
{
    w25q64_status_t ret = W25Q64_ERROR_UNKNOWN;

    /* Check required interfaces */
    if (NULL == p_w25q64_instance->p_os_delay_instance)
    {
        return W25Q64_ERROR_RESOURCE;
    }
    if ((NULL == p_w25q64_instance->p_spi_w25q64_instance) ||
        (NULL == p_w25q64_instance->p_spi_w25q64_instance->pf_spi_init))
    {
        return W25Q64_ERROR_RESOURCE;
    }
    if (NULL == p_w25q64_instance->p_private_data)
    {
        return W25Q64_ERROR_RESOURCE;
    }

    /* Protect the complete initialization sequence */
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_init(NULL);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();

    /* Wake up before reading the ID */
    p_w25q64_instance->pf_wakeup(p_w25q64_instance);
    ret = __w25q64read_id(p_w25q64_instance);
    if (W25Q64_OK != ret)
    {
        return ret;
    }
    p_w25q64_instance->p_private_data->w25q64_inited = W25Q64_INITED;

    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    return ret;
}

/**
 * @brief 去初始化 W25Q64 驱动。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @return w25q64_status_t : 去初始化状态码。
 */
static w25q64_status_t deinit(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    /* Reject operation before initialization */
    if (!W25Q64_ISINITED)
    {
        return W25Q64_ERROR_RESOURCE;
    }
    if (NULL == p_w25q64_instance->p_spi_w25q64_instance->pf_spi_deinit)
    {
        return W25Q64_ERROR_RESOURCE;
    }

    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_deinit(NULL);
    p_w25q64_instance->p_private_data->w25q64_inited = W25Q64_UNINITED;
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    return W25Q64_OK;
}

/**
 * @brief 让 W25Q64 进入掉电睡眠状态。
 *
 * Steps:
 *  1. 等待芯片空闲。
 *  2. 发送睡眠命令。
 *  3. 更新驱动睡眠状态。
 *
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @return w25q64_status_t : 操作状态码。
 */
static w25q64_status_t sleep(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    uint8_t busy;

    /* Already in sleep state */
    if (W25Q64_SLEEP == p_w25q64_instance->p_private_data->w25q64_isSleep)
    {
        return W25Q64_ERROR_RESOURCE;
    }

    /* Wait until BUSY is cleared */
    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    /* Send SLEEP command */
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_SLEEP);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_private_data->w25q64_isSleep = W25Q64_SLEEP;
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    return W25Q64_OK;
}

/**
 * @brief 唤醒 W25Q64 芯片并发送设备 ID。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @return w25q64_status_t : 操作状态码。
 */
static w25q64_status_t wakeup(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    uint8_t id;

    /* Already in wakeup state */
    if (W25Q64_WAKEUP == p_w25q64_instance->p_private_data->w25q64_isSleep)
    {
        return W25Q64_ERROR_RESOURCE;
    }

    /* Send WAKEUP command and device ID */
    id = (uint8_t)(p_w25q64_instance->p_private_data->w25qxx_id & 0x00FF);
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_WAKEUP);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(0x00);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(0x00);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(0x00);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(id);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_private_data->w25q64_isSleep = W25Q64_WAKEUP;
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    return W25Q64_OK;
}

/**
 * @brief 读取缓存的 W25Q64 芯片 ID。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @param[out] p_id              : 芯片 ID 输出指针。
 * @return w25q64_status_t : 读取状态码。
 */
w25q64_status_t read_w25qxx_id(bsp_w25q64_driver_t *const p_w25q64_instance,
                               uint16_t *const p_id)
{
    if (!W25Q64_ISINITED)
    {
        return W25Q64_ERROR_RESOURCE;
    }
    *p_id = p_w25q64_instance->p_private_data->w25qxx_id;
    return W25Q64_OK;
}

/**
 * @brief 读取 W25Q64 当前睡眠状态。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @return w25q64_status_t : 当前睡眠状态或错误码。
 */
w25q64_status_t read_w25qxx_isSleep(
    bsp_w25q64_driver_t *const p_w25q64_instance)
{
    if (!W25Q64_ISINITED)
    {
        return W25Q64_ERROR_RESOURCE;
    }
    return p_w25q64_instance->p_private_data->w25q64_isSleep;
}

/**
 * @brief 从 W25Q64 连续读取数据。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @param[out] p_data            : 数据接收缓冲区。
 * @param[in] read_addr          : 读取起始地址。
 * @param[in] read_len           : 读取字节数。
 * @return w25q64_status_t : 读取状态码。
 */
static w25q64_status_t read_data(bsp_w25q64_driver_t *const p_w25q64_instance,
                                 uint8_t *p_data,
                                 uint32_t read_addr,
                                 uint32_t read_len)
{
    uint8_t busy;

    if (NULL == p_data)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    /* Wait until BUSY is cleared */
    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    /* Check address range */
    if (read_addr + read_len > W25Q64_MAX_SIZE)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    /* Send READ_DATA command, address, and data */
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_READ_DATA);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte((uint8_t)(read_addr >> 16));
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte((uint8_t)(read_addr >> 8));
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte((uint8_t)(read_addr));
    while (read_len--)
    {
        *p_data = p_w25q64_instance->p_spi_w25q64_instance->pf_spi_read_byte();
        p_data++;
    }
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    return W25Q64_OK;
}

/**
 * @brief 擦除一个 4 KB Flash 扇区。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @param[in] sector_addr       : 扇区起始地址，需按 4 KB 对齐。
 * @return w25q64_status_t : 擦除状态码。
 */
static w25q64_status_t __sector_erase(
    bsp_w25q64_driver_t *const p_w25q64_instance, uint32_t sector_addr)
{
    uint8_t busy;

    /* Check 4 KB alignment */
    if (0 != (sector_addr & 0x00000FFFU))
    {
        return W25Q64_ERROR_PARAMETER;
    }

    /* Wait until BUSY is cleared */
    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    /* Send write-enable command */
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_WRITEON_REG);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    /* Wait until write-enable completes */
    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    /* Send sector-erase command and address */
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_ERASE_SECTOR);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte((uint8_t)((sector_addr & 0x00FF0000U) >> 16));
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte((uint8_t)((sector_addr & 0x0000FF00U) >> 8));
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte((uint8_t)(sector_addr & 0x000000FFU));
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    return W25Q64_OK;
}

/**
 * @brief 擦除整颗 W25Q64 芯片。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @return w25q64_status_t : 擦除状态码。
 */
static w25q64_status_t chip_erase(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    uint8_t busy;

    /* Wait until BUSY is cleared */
    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    /* Send write-enable command */
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_WRITEON_REG);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    /* Wait until BUSY is cleared */
    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    /* Send chip-erase command */
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_CHIP_SECTOR);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    return W25Q64_OK;
}

/**
 * @brief 按页写入数据，自动处理跨页写入。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @param[in] p_data            : 待写入数据。
 * @param[in] write_addr        : 写入起始地址。
 * @param[in] size              : 写入字节数。
 * @return w25q64_status_t : 写入状态码。
 */
static w25q64_status_t __write(bsp_w25q64_driver_t *const p_w25q64_instance,
                               const uint8_t *p_data,
                               uint32_t write_addr,
                               uint32_t size)
{
    uint32_t page_offset;
    uint32_t page_write_len;

    /* Check parameters */
    if (NULL == p_data)
    {
        return W25Q64_ERROR_PARAMETER;
    }
    if (write_addr + size > W25Q64_MAX_SIZE)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    /* Calculate remaining space in the current page */
    page_offset = W25Q64_PAGE_SIZE - (write_addr % W25Q64_PAGE_SIZE);
    while (1)
    {
        uint8_t busy;

        /* Limit the final segment to the remaining length */
        if (size <= page_offset)
        {
            page_offset = size;
        }
        page_write_len = page_offset;

        /* Wait until BUSY is cleared */
        busy = __read_status(p_w25q64_instance);
        while (busy)
        {
            p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(1);
            busy = __read_status(p_w25q64_instance);
        }

        /* Send write-enable command */
        p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_WRITEON_REG);
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
        p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

        /* Wait until write-enable completes */
        busy = __read_status(p_w25q64_instance);
        while (busy)
        {
            p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(1);
            busy = __read_status(p_w25q64_instance);
        }

        /* Send page-write command, address, and data */
        p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_WRITE_DATA);
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte((uint8_t)((write_addr & 0x00FF0000U) >> 16));
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte((uint8_t)((write_addr & 0x0000FF00U) >> 8));
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte((uint8_t)(write_addr & 0x000000FFU));
        while (page_write_len--)
        {
            p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(*p_data);
            p_data++;
        }
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
        p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

        /* Current page write completed */
        if (page_offset == size)
        {
            break;
        }
        write_addr  += page_offset;
        size        -= page_offset;
        page_offset  = W25Q64_PAGE_SIZE;
    }
    return W25Q64_OK;
}

/**
 * @brief 按扇区擦除后写入数据。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @param[in] p_data            : 待写入数据。
 * @param[in] write_addr        : 写入起始地址。
 * @param[in] size              : 写入字节数。
 * @return w25q64_status_t : 写入状态码。
 */
static w25q64_status_t write_data_erase(
    bsp_w25q64_driver_t *const p_w25q64_instance,
    const uint8_t *p_data,
    uint32_t write_addr,
    uint32_t size)
{
    uint32_t sector_offset;
    uint32_t sector_remain;
    uint32_t sector_addr;

    /* Check parameters */
    if (NULL == p_data)
    {
        return W25Q64_ERROR_PARAMETER;
    }
    if (write_addr + size > W25Q64_MAX_SIZE)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    /* Calculate remaining space and start address */
    sector_offset = write_addr % W25Q64_SECTOR_SIZE;
    sector_remain = W25Q64_SECTOR_SIZE - sector_offset;
    sector_addr   = write_addr - sector_offset;

    while (1)
    {
        uint8_t status;

        /* Limit the final segment to the remaining length */
        if (size <= sector_remain)
        {
            sector_remain = size;
        }

        /* Erase the current sector and wait for completion */
        __sector_erase(p_w25q64_instance, sector_addr);
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(1);

        status = __read_status(p_w25q64_instance);
        while (status)
        {
            p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(1);
            status = __read_status(p_w25q64_instance);
        }

        /* Write data to the current sector */
        __write(p_w25q64_instance, p_data, write_addr, sector_remain);

        /* All data has been written */
        if (sector_remain == size)
        {
            break;
        }

        /* Move to the next sector */
        p_data        += sector_remain;
        size          -= sector_remain;
        write_addr    += sector_remain;
        sector_addr    = write_addr;
        sector_remain  = W25Q64_SECTOR_SIZE;
    }
    return W25Q64_OK;
}

/**
 * @brief 不擦除 Flash，直接写入数据。
 * @param[in] p_w25q64_instance : W25Q64 驱动实例。
 * @param[in] p_data            : 待写入数据。
 * @param[in] write_addr        : 写入起始地址。
 * @param[in] size              : 写入字节数。
 * @return w25q64_status_t : 写入状态码。
 */
static w25q64_status_t write_data_noErase(
    bsp_w25q64_driver_t *const p_w25q64_instance,
    const uint8_t *p_data,
    uint32_t write_addr,
    uint32_t size)
{
    uint32_t sector_offset;
    uint32_t sector_remain;
    uint8_t  status;

    /* Check parameters */
    if (NULL == p_data)
    {
        return W25Q64_ERROR_PARAMETER;
    }
    if (write_addr + size > W25Q64_MAX_SIZE)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    /* Reject writes crossing a sector boundary */
    sector_offset = write_addr % W25Q64_SECTOR_SIZE;
    sector_remain = W25Q64_SECTOR_SIZE - sector_offset;
    if (size > sector_remain)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    /* Wait until BUSY is cleared */
    status = __read_status(p_w25q64_instance);
    while (status)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(150);
        status = __read_status(p_w25q64_instance);
    }

    /* Write pages directly */
    __write(p_w25q64_instance, p_data, write_addr, size);
    return W25Q64_OK;
}

/**
 * @brief 实例化 W25Q64 驱动并完成初始化。
 * @param[in] p_w25q64_instance       : W25Q64 驱动实例。
 * @param[in] p_spi_w25q64_instance   : SPI 接口。
 * @param[in] p_timebase_instance     : 时基接口。
 * @param[in] p_os_delay_instance     : 延时接口。
 * @return w25q64_status_t : 实例化状态码。
 */
w25q64_status_t w25q64_inst(
    bsp_w25q64_driver_t         *const p_w25q64_instance,
    spi_w25q64_interface_t      *const p_spi_w25q64_instance,
    w25qxx_timebase_interface_t *const p_timebase_instance,
    w25qxx_os_delay_interface_t *const p_os_delay_instance)
{
    w25q64_status_t ret = W25Q64_ERROR_UNKNOWN;
    w25q64_private_data_t *p_private_data;

    /* Allocate private data */
    p_private_data =
        (w25q64_private_data_t *)malloc(sizeof(w25q64_private_data_t));
    if (NULL == p_private_data)
    {
        return W25Q64_ERROR_NOMEM;
    }
    memset(p_private_data, 0, sizeof(w25q64_private_data_t));
    p_w25q64_instance->p_private_data = p_private_data;

    /* Reject repeated initialization */
    if (W25Q64_ISINITED)
    {
        return W25Q64_ERROR_RESOURCE;
    }

    /* Check parameters */
    if ((NULL == p_w25q64_instance) ||
        (NULL == p_spi_w25q64_instance) ||
        (NULL == p_timebase_instance))
    {
        return W25Q64_ERROR_PARAMETER;
    }
    if (NULL == p_os_delay_instance)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    /* Attach external interfaces */
    p_w25q64_instance->p_spi_w25q64_instance = p_spi_w25q64_instance;
    p_w25q64_instance->p_timebase_instance   = p_timebase_instance;
    p_w25q64_instance->p_os_delay_instance   = p_os_delay_instance;

    /* Attach driver function pointers */
    p_w25q64_instance->pf_init          = __w25q64init;
    p_w25q64_instance->pf_deinit        = deinit;
    p_w25q64_instance->pf_read          = read_data;
    p_w25q64_instance->pf_write_erase   = write_data_erase;
    p_w25q64_instance->pf_write_no_erase = write_data_noErase;
    p_w25q64_instance->pf_erase_chip    = chip_erase;
    p_w25q64_instance->pf_erase_sector  = (w25q64_status_t (*)(void *const, uint32_t))__sector_erase;
    p_w25q64_instance->pf_sleep         = sleep;
    p_w25q64_instance->pf_wakeup        = wakeup;

    /* Initialize the chip and release data on failure */
    ret = p_w25q64_instance->pf_init(p_w25q64_instance);
    if (W25Q64_OK != ret)
    {
        free(p_private_data);
    }
    return ret;
}
