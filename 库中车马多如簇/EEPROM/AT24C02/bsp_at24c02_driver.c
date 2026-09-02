#include "bsp_at24c02_driver.h"
#include <stdlib.h>
#include <string.h>

/*
 * AT24C02 驱动实现（寄存器操作）
 * 所有 I2C 访问都通过注入的 at24c02_i2c_interface_t 接口完成，
 * I2C 总线互斥由 I2C Port 的锁回调保证，Driver 不关心。
 */

#define AT24C02_INITED   (1U)
#define AT24C02_UNINITED (0U)

typedef struct at24c02_private_data
{
    uint8_t inited;
    uint8_t device_address_7bit;
} at24c02_private_data_t;

/* ACK 轮询：探测器件是否就绪（写周期结束） */
static at24c02_status_t at24c02_probe(bsp_at24c02_driver_t *const p_instance)
{
    return p_instance->p_i2c_instance->pf_i2c_is_ready(
        p_instance->p_private_data->device_address_7bit,
        AT24C02_READY_TRIALS, AT24C02_I2C_TIMEOUT_MS);
}

/* 校验实例已初始化且接口注入完整 */
static at24c02_status_t at24c02_validate(bsp_at24c02_driver_t *const p_instance)
{
    if ((p_instance == NULL) || (p_instance->p_private_data == NULL) ||
        (p_instance->p_i2c_instance == NULL))
    {
        return AT24C02_ERROR_PARAMETER;
    }
    if (p_instance->p_private_data->inited != AT24C02_INITED)
    {
        return AT24C02_ERROR_NOT_READY;
    }
    return AT24C02_OK;
}

/* 校验读写参数：地址 + 长度不越界，缓冲非空 */
static at24c02_status_t at24c02_validate_transfer(
    bsp_at24c02_driver_t *const p_instance,
    uint32_t addr, const void *data, uint32_t size)
{
    at24c02_status_t status = at24c02_validate(p_instance);

    if (status != AT24C02_OK)
    {
        return status;
    }
    if ((data == NULL) || (size == 0U) ||
        (addr >= AT24C02_CAPACITY_BYTES) ||
        (size > (AT24C02_CAPACITY_BYTES - addr)))
    {
        return AT24C02_ERROR_PARAMETER;
    }
    return AT24C02_OK;
}

/* 初始化：探测器件，就绪后置 inited */
static at24c02_status_t at24c02_init(bsp_at24c02_driver_t *const p_instance)
{
    at24c02_status_t status;

    if ((p_instance == NULL) || (p_instance->p_private_data == NULL))
    {
        return AT24C02_ERROR_PARAMETER;
    }

    status = at24c02_probe(p_instance);
    if (status == AT24C02_OK)
    {
        p_instance->p_private_data->inited = AT24C02_INITED;
    }
    return status;
}

/* 去初始化 */
static at24c02_status_t at24c02_deinit(bsp_at24c02_driver_t *const p_instance)
{
    if ((p_instance == NULL) || (p_instance->p_private_data == NULL))
    {
        return AT24C02_ERROR_PARAMETER;
    }
    p_instance->p_private_data->inited = AT24C02_UNINITED;
    return AT24C02_OK;
}

/* 连续读 */
static at24c02_status_t at24c02_read(bsp_at24c02_driver_t *const p_instance,
                                     uint8_t *p_data, uint32_t addr, uint32_t size)
{
    at24c02_status_t status = at24c02_validate_transfer(p_instance, addr,
                                                        p_data, size);

    if (status != AT24C02_OK)
    {
        return status;
    }
    return p_instance->p_i2c_instance->pf_i2c_mem_read(
        p_instance->p_private_data->device_address_7bit,
        (uint16_t)addr, p_data, (uint16_t)size, AT24C02_I2C_TIMEOUT_MS);
}

/* 连续写：自动按 8 字节页拆分，每页写后 ACK 轮询等待写周期 */
static at24c02_status_t at24c02_write(bsp_at24c02_driver_t *const p_instance,
                                      const uint8_t *p_data, uint32_t addr,
                                      uint32_t size)
{
    at24c02_status_t status = at24c02_validate_transfer(p_instance, addr,
                                                        p_data, size);
    uint16_t mem_addr = (uint16_t)addr;
    uint16_t remaining = (uint16_t)size;

    if (status != AT24C02_OK)
    {
        return status;
    }

    while (remaining > 0U)
    {
        uint16_t page_remaining = (uint16_t)(AT24C02_PAGE_SIZE -
                                             (mem_addr % AT24C02_PAGE_SIZE));
        uint16_t chunk = (remaining < page_remaining) ? remaining : page_remaining;

        status = p_instance->p_i2c_instance->pf_i2c_mem_write(
            p_instance->p_private_data->device_address_7bit,
            mem_addr, p_data, chunk, AT24C02_I2C_TIMEOUT_MS);
        if (status != AT24C02_OK)
        {
            return status;
        }

        status = at24c02_probe(p_instance);
        if (status != AT24C02_OK)
        {
            return status;
        }

        p_data += chunk;
        mem_addr += chunk;
        remaining -= chunk;
    }

    return AT24C02_OK;
}

/* 全片写 0xFF（EEPROM 擦除态） */
static at24c02_status_t at24c02_erase(bsp_at24c02_driver_t *const p_instance)
{
    uint16_t addr;
    static const uint8_t erased_page[AT24C02_PAGE_SIZE] = {
        0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU
    };
    at24c02_status_t status = at24c02_validate(p_instance);

    if (status != AT24C02_OK)
    {
        return status;
    }

    for (addr = 0U; addr < AT24C02_CAPACITY_BYTES; addr += AT24C02_PAGE_SIZE)
    {
        status = at24c02_write(p_instance, erased_page, (uint32_t)addr,
                               (uint32_t)sizeof(erased_page));
        if (status != AT24C02_OK)
        {
            return status;
        }
    }

    return AT24C02_OK;
}

/* 实例化驱动：挂载接口 + 函数指针 + 初始化探测 */
at24c02_status_t at24c02_inst(bsp_at24c02_driver_t *const p_instance,
                              at24c02_i2c_interface_t *const p_i2c_instance,
                              uint8_t address_pins)
{
    at24c02_private_data_t *p_private_data;

    if ((p_instance == NULL) || (p_i2c_instance == NULL))
    {
        return AT24C02_ERROR_PARAMETER;
    }
    if (address_pins > AT24C02_ADDRESS_PINS_MAX)
    {
        return AT24C02_ERROR_PARAMETER;
    }

    p_private_data = (at24c02_private_data_t *)malloc(sizeof(at24c02_private_data_t));
    if (p_private_data == NULL)
    {
        return AT24C02_ERROR_RESOURCE;
    }
    memset(p_private_data, 0, sizeof(at24c02_private_data_t));
    p_private_data->device_address_7bit =
        (uint8_t)(AT24C02_BASE_ADDRESS_7BIT | address_pins);

    p_instance->p_i2c_instance = p_i2c_instance;
    p_instance->p_private_data = p_private_data;

    p_instance->pf_inst = at24c02_inst;
    p_instance->pf_init = at24c02_init;
    p_instance->pf_deinit = at24c02_deinit;
    p_instance->pf_read = at24c02_read;
    p_instance->pf_write = at24c02_write;
    p_instance->pf_erase = at24c02_erase;

    return p_instance->pf_init(p_instance);
}
