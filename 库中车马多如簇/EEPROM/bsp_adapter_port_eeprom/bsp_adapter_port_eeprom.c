#include "bsp_adapter_port_eeprom.h"

#include "bsp_at24c02_driver.h"
#include "bsp_wrapper_eeprom.h"
#include "i2c_port.h"

/*
 * EEPROM Adapter 实现
 *
 * 把 eeprom_drv_t 函数表接到 AT24C02 Driver（同步调用）：
 *   init -> at24c02_inst（探测器件）
 *   read/write/erase -> 直接调 g_at24c02Driver 的 pf_* 函数同步返回结果。
 *
 * 所有 I2C 依赖通过函数指针注入，本层不把 I2C Port 类型暴露给 Driver。
 */

/* ===== 桥接函数：学习库 core_i2c -> driver 期望的函数指针签名 ===== */

static at24c02_status_t eeprom_i2c_mem_read(uint8_t device_address_7bit,
                                            uint16_t memory_address,
                                            uint8_t *data,
                                            uint16_t size,
                                            uint32_t timeout_ms)
{
    return (core_i2c_mem_read((core_i2c_port_id_t)EEPROM_I2C_PORT,
                              device_address_7bit,
                              memory_address,
                              CORE_I2C_MEM_ADDR_8BIT,
                              data, size, timeout_ms) == CORE_I2C_OK)
               ? AT24C02_OK : AT24C02_ERROR;
}

static at24c02_status_t eeprom_i2c_mem_write(uint8_t device_address_7bit,
                                             uint16_t memory_address,
                                             const uint8_t *data,
                                             uint16_t size,
                                             uint32_t timeout_ms)
{
    return (core_i2c_mem_write((core_i2c_port_id_t)EEPROM_I2C_PORT,
                               device_address_7bit,
                               memory_address,
                               CORE_I2C_MEM_ADDR_8BIT,
                               data, size, timeout_ms) == CORE_I2C_OK)
               ? AT24C02_OK : AT24C02_ERROR;
}

static at24c02_status_t eeprom_i2c_is_ready(uint8_t device_address_7bit,
                                            uint32_t trials,
                                            uint32_t timeout_ms)
{
    en_core_i2c_status_t status =
        core_i2c_is_ready((core_i2c_port_id_t)EEPROM_I2C_PORT,
                          device_address_7bit, trials, timeout_ms);

    switch (status)
    {
        case CORE_I2C_OK:
            return AT24C02_OK;
        case CORE_I2C_ERR_NACK:
            return AT24C02_ERROR_NOT_READY;
        case CORE_I2C_ERR_TIMEOUT:
            return AT24C02_ERROR_TIMEOUT;
        default:
            return AT24C02_ERROR;
    }
}

/* ===== 接口实例 ===== */

static at24c02_i2c_interface_t g_at24c02I2cInterface = {
    .pf_i2c_mem_read = eeprom_i2c_mem_read,
    .pf_i2c_mem_write = eeprom_i2c_mem_write,
    .pf_i2c_is_ready = eeprom_i2c_is_ready,
};

static bsp_at24c02_driver_t g_at24c02Driver;

/* ===== 状态映射：driver 状态码 -> wrapper 统一状态码 ===== */

static eeprom_status_t eeprom_map_status(at24c02_status_t status)
{
    switch (status)
    {
        case AT24C02_OK:
            return EEPROM_OK;
        case AT24C02_ERROR_TIMEOUT:
            return EEPROM_ERROR_TIMEOUT;
        case AT24C02_ERROR_RESOURCE:
            return EEPROM_ERROR_RESOURCE;
        case AT24C02_ERROR_PARAMETER:
            return EEPROM_ERROR_PARAMETER;
        case AT24C02_ERROR_NOT_READY:
            return EEPROM_ERROR_NOT_READY;
        default:
            return EEPROM_ERROR;
    }
}

/* ===== eeprom_drv_t 函数表：同步调用 driver ===== */

static eeprom_status_t eeprom_drv_init(eeprom_drv_t *dev)
{
    (void)dev;
    return eeprom_map_status(at24c02_inst(&g_at24c02Driver,
                                          &g_at24c02I2cInterface,
                                          (uint8_t)EEPROM_DEVICE_ADDRESS_PINS));
}

static eeprom_status_t eeprom_drv_deinit(eeprom_drv_t *dev)
{
    (void)dev;
    return eeprom_map_status(g_at24c02Driver.pf_deinit(&g_at24c02Driver));
}

static eeprom_status_t eeprom_drv_read(eeprom_drv_t *dev, uint32_t addr,
                                       uint32_t size, uint8_t *p_buf)
{
    (void)dev;
    return eeprom_map_status(g_at24c02Driver.pf_read(&g_at24c02Driver,
                                                     p_buf, addr, size));
}

static eeprom_status_t eeprom_drv_write(eeprom_drv_t *dev, uint32_t addr,
                                        uint32_t size, uint8_t *p_buf)
{
    (void)dev;
    return eeprom_map_status(g_at24c02Driver.pf_write(&g_at24c02Driver,
                                                      p_buf, addr, size));
}

static eeprom_status_t eeprom_drv_erase(eeprom_drv_t *dev)
{
    (void)dev;
    return eeprom_map_status(g_at24c02Driver.pf_erase(&g_at24c02Driver));
}

/* ===== 注册入口 ===== */

void bsp_adapter_eeprom_register(void)
{
    eeprom_drv_t eepromDrv = {
        .idx = 0U,
        .dev_id = 0U,
        .eeprom_drv_init = eeprom_drv_init,
        .eeprom_drv_deinit = eeprom_drv_deinit,
        .eeprom_drv_read = eeprom_drv_read,
        .eeprom_drv_write = eeprom_drv_write,
        .eeprom_drv_erase = eeprom_drv_erase,
    };

    (void)bsp_wrapper_eeprom_reg(0U, &eepromDrv);
}
