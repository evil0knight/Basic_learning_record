#include "bsp_w25q64_driver.h"
#include <stdlib.h>
#include <string.h>

/*
 * W25Q64 驱动实现（照手表 bsp_w25q64_driver.c 搬运）
 * 所有 SPI 访问都通过注入的 spi_w25q64_interface_t 逐字节接口完成，
 * OS 临界区用 pf_critical_enter/exit 保护 CS 期间数据完整。
 */

#ifndef W25Q64_LOG
#define W25Q64_LOG(...) ((void)0)
#endif

#define W25Q64_INITED   (1U)
#define W25Q64_UNINITED (0U)

#define W25Q64_ISINITED \
    (W25Q64_INITED == (p_w25q64_instance->p_private_data->w25q64_inited))

/* W25Q64 驱动私有数据 */
typedef struct w25q64_private_data
{
    uint8_t         w25q64_inited;
    uint16_t        w25qxx_id;
    w25q64_status_t w25q64_isSleep;
} w25q64_private_data_t;

/* 读状态寄存器，返回 0=不忙，非 0=忙 */
static uint8_t __read_status(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    uint8_t status = 0;

    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_READ_REG);
    status = p_w25q64_instance->p_spi_w25q64_instance->pf_spi_read_byte();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    return (1 == (status & 0x01)) ? 6U : 0U;
}

/* 读 W25Q64 ID */
static w25q64_status_t __w25q64read_id(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    uint16_t id = 0;

    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_READ_ID);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(0x00);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(0x00);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(0x00);
    id = p_w25q64_instance->p_spi_w25q64_instance->pf_spi_read_byte();
    id <<= 8;
    id |= p_w25q64_instance->p_spi_w25q64_instance->pf_spi_read_byte();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    if (0xEF16 == id)
    {
        p_w25q64_instance->p_private_data->w25qxx_id = id;
        return W25Q64_OK;
    }
    W25Q64_LOG("Read W25Q64 ID failed\n");
    return W25Q64_ERROR_RESOURCE;
}

/* 初始化驱动 */
static w25q64_status_t __w25q64init(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    w25q64_status_t ret = W25Q64_ERROR_UNKNOWN;

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

    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_init(NULL);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();

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

/* 去初始化 */
static w25q64_status_t deinit(bsp_w25q64_driver_t *const p_w25q64_instance)
{
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

/* 睡眠 */
static w25q64_status_t sleep(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    uint8_t busy;

    if (W25Q64_SLEEP == p_w25q64_instance->p_private_data->w25q64_isSleep)
    {
        return W25Q64_ERROR_RESOURCE;
    }

    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_SLEEP);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_private_data->w25q64_isSleep = W25Q64_SLEEP;
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    return W25Q64_OK;
}

/* 唤醒 */
static w25q64_status_t wakeup(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    uint8_t id;

    if (W25Q64_WAKEUP == p_w25q64_instance->p_private_data->w25q64_isSleep)
    {
        return W25Q64_ERROR_RESOURCE;
    }

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

/* 读 ID */
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

/* 读睡眠状态 */
w25q64_status_t read_w25qxx_isSleep(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    if (!W25Q64_ISINITED)
    {
        return W25Q64_ERROR_RESOURCE;
    }
    return p_w25q64_instance->p_private_data->w25q64_isSleep;
}

/* 读数据 */
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

    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    if (read_addr + read_len > W25Q64_MAX_SIZE)
    {
        return W25Q64_ERROR_PARAMETER;
    }

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

/* 擦除一个 4KB 扇区 */
static w25q64_status_t __sector_erase(bsp_w25q64_driver_t *const p_w25q64_instance,
                                      uint32_t sector_addr)
{
    uint8_t busy;

    if ((sector_addr & 0x00000FFFU) != 0)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    /* 写使能 */
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_WRITEON_REG);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    /* 擦除扇区命令 + 24bit 地址 */
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

/* 全片擦除 */
static w25q64_status_t chip_erase(bsp_w25q64_driver_t *const p_w25q64_instance)
{
    uint8_t busy;

    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_WRITEON_REG);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    busy = __read_status(p_w25q64_instance);
    while (busy)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(50);
        busy = __read_status(p_w25q64_instance);
    }

    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_CHIP_SECTOR);
    p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
    p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

    return W25Q64_OK;
}

/* 页写（内部，单页内） */
static w25q64_status_t __write(bsp_w25q64_driver_t *const p_w25q64_instance,
                               const uint8_t *p_data,
                               uint32_t write_addr,
                               uint32_t size)
{
    uint32_t page_offset;
    uint32_t page_write_len;

    if (NULL == p_data)
    {
        return W25Q64_ERROR_PARAMETER;
    }
    if (write_addr + size > W25Q64_MAX_SIZE)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    page_offset = W25Q64_PAGE_SIZE - (write_addr % W25Q64_PAGE_SIZE);
    while (1)
    {
        uint8_t busy;

        if (size <= page_offset)
        {
            page_offset = size;
        }
        page_write_len = page_offset;

        busy = __read_status(p_w25q64_instance);
        while (busy)
        {
            p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(1);
            busy = __read_status(p_w25q64_instance);
        }

        /* 写使能 */
        p_w25q64_instance->p_spi_w25q64_instance->pf_critical_enter();
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_set();
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_write_byte(W25Q64_CMD_WRITEON_REG);
        p_w25q64_instance->p_spi_w25q64_instance->pf_spi_cs_deset();
        p_w25q64_instance->p_spi_w25q64_instance->pf_critical_exit();

        busy = __read_status(p_w25q64_instance);
        while (busy)
        {
            p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(1);
            busy = __read_status(p_w25q64_instance);
        }

        /* 写数据命令 + 24bit 地址 + 数据 */
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

        if (page_offset == size)
        {
            break;
        }
        write_addr += page_offset;
        size -= page_offset;
        page_offset = W25Q64_PAGE_SIZE;
    }
    return W25Q64_OK;
}

/* 擦除 + 写（按扇区） */
static w25q64_status_t write_data_erase(bsp_w25q64_driver_t *const p_w25q64_instance,
                                        const uint8_t *p_data,
                                        uint32_t write_addr,
                                        uint32_t size)
{
    uint32_t sector_offset;
    uint32_t sector_remain;
    uint32_t sector_addr;

    if (NULL == p_data)
    {
        return W25Q64_ERROR_PARAMETER;
    }
    if (write_addr + size > W25Q64_MAX_SIZE)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    sector_offset = write_addr % W25Q64_SECTOR_SIZE;
    sector_remain = W25Q64_SECTOR_SIZE - sector_offset;
    sector_addr = write_addr - sector_offset;

    while (1)
    {
        uint8_t status;

        if (size <= sector_remain)
        {
            sector_remain = size;
        }
        __sector_erase(p_w25q64_instance, sector_addr);
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(1);

        status = __read_status(p_w25q64_instance);
        while (status)
        {
            p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(1);
            status = __read_status(p_w25q64_instance);
        }

        __write(p_w25q64_instance, p_data, write_addr, sector_remain);

        if (sector_remain == size)
        {
            break;
        }
        p_data += sector_remain;
        size -= sector_remain;
        write_addr += sector_remain;
        sector_addr = write_addr;
        sector_remain = W25Q64_SECTOR_SIZE;
    }
    return W25Q64_OK;
}

/* 不擦除写（单扇区内） */
static w25q64_status_t write_data_noErase(bsp_w25q64_driver_t *const p_w25q64_instance,
                                          const uint8_t *p_data,
                                          uint32_t write_addr,
                                          uint32_t size)
{
    uint32_t sector_offset;
    uint32_t sector_remain;
    uint8_t status;

    if (NULL == p_data)
    {
        return W25Q64_ERROR_PARAMETER;
    }
    if (write_addr + size > W25Q64_MAX_SIZE)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    sector_offset = write_addr % W25Q64_SECTOR_SIZE;
    sector_remain = W25Q64_SECTOR_SIZE - sector_offset;

    if (size > sector_remain)
    {
        return W25Q64_ERROR_PARAMETER;
    }

    status = __read_status(p_w25q64_instance);
    while (status)
    {
        p_w25q64_instance->p_os_delay_instance->pf_os_delay_ms(150);
        status = __read_status(p_w25q64_instance);
    }

    __write(p_w25q64_instance, p_data, write_addr, size);
    return W25Q64_OK;
}

/* 实例化驱动：挂载接口 + 函数指针 + 初始化 */
w25q64_status_t w25q64_inst(
    bsp_w25q64_driver_t      *const p_w25q64_instance,
    spi_w25q64_interface_t   *const p_spi_w25q64_instance,
    w25qxx_timebase_interface_t *const p_timebase_instance,
    w25qxx_os_delay_interface_t *const p_os_delay_instance)
{
    w25q64_status_t ret = W25Q64_ERROR_UNKNOWN;
    w25q64_private_data_t *p_private_data;

    p_private_data = (w25q64_private_data_t *)malloc(sizeof(w25q64_private_data_t));
    if (NULL == p_private_data)
    {
        return W25Q64_ERROR_NOMEM;
    }
    memset(p_private_data, 0, sizeof(w25q64_private_data_t));
    p_w25q64_instance->p_private_data = p_private_data;

    if (W25Q64_ISINITED)
    {
        return W25Q64_ERROR_RESOURCE;
    }
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

    p_w25q64_instance->p_spi_w25q64_instance = p_spi_w25q64_instance;
    p_w25q64_instance->p_timebase_instance = p_timebase_instance;
    p_w25q64_instance->p_os_delay_instance = p_os_delay_instance;

    p_w25q64_instance->pf_init = __w25q64init;
    p_w25q64_instance->pf_deinit = deinit;
    p_w25q64_instance->pf_read = read_data;
    p_w25q64_instance->pf_write_erase = write_data_erase;
    p_w25q64_instance->pf_write_no_erase = write_data_noErase;
    p_w25q64_instance->pf_erase_chip = chip_erase;
    p_w25q64_instance->pf_sleep = sleep;
    p_w25q64_instance->pf_wakeup = wakeup;

    ret = p_w25q64_instance->pf_init(p_w25q64_instance);
    if (W25Q64_OK != ret)
    {
        free(p_private_data);
    }
    return ret;
}
