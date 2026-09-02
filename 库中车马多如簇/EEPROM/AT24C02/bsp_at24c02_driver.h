#ifndef BSP_AT24C02_DRIVER_H
#define BSP_AT24C02_DRIVER_H

#include <stdint.h>
#include "bsp_at24c02_reg.h"

/*
 * AT24C02 驱动层
 * 依赖全部通过函数指针接口注入，不直接依赖 OS / I2C 类型：
 *   - at24c02_i2c_interface_t：I2C 逐字节接口（由 Adapter 用 core_i2c 实现）
 */

/* AT24C02 状态返回码 */
typedef enum
{
    AT24C02_OK = 0,
    AT24C02_ERROR,
    AT24C02_ERROR_TIMEOUT,
    AT24C02_ERROR_RESOURCE,
    AT24C02_ERROR_PARAMETER,
    AT24C02_ERROR_NOT_READY,
    AT24C02_RESERVED = 0x1FFFFFFF
} at24c02_status_t;

/* ===== I2C 逐字节接口（来自 I2C Port，由 Adapter 适配注入）===== */

typedef struct
{
    at24c02_status_t (*pf_i2c_mem_read)(uint8_t device_address_7bit,
                                        uint16_t memory_address,
                                        uint8_t *data,
                                        uint16_t size,
                                        uint32_t timeout_ms);
    at24c02_status_t (*pf_i2c_mem_write)(uint8_t device_address_7bit,
                                         uint16_t memory_address,
                                         const uint8_t *data,
                                         uint16_t size,
                                         uint32_t timeout_ms);
    at24c02_status_t (*pf_i2c_is_ready)(uint8_t device_address_7bit,
                                        uint32_t trials,
                                        uint32_t timeout_ms);
} at24c02_i2c_interface_t;

/* ===== AT24C02 驱动实例 ===== */

typedef struct at24c02_private_data at24c02_private_data_t;

typedef struct
{
    /* 注入的外部接口 */
    at24c02_i2c_interface_t *p_i2c_instance;
    at24c02_private_data_t  *p_private_data;

    /* 实例函数指针（实例化时挂载） */
    at24c02_status_t (*pf_inst)(void *const,
                                at24c02_i2c_interface_t *const,
                                uint8_t);
    at24c02_status_t (*pf_init)(void *const);
    at24c02_status_t (*pf_deinit)(void *const);
    at24c02_status_t (*pf_read)(void *const, uint8_t *, uint32_t, uint32_t);
    at24c02_status_t (*pf_write)(void *const, const uint8_t *, uint32_t, uint32_t);
    at24c02_status_t (*pf_erase)(void *const);
} bsp_at24c02_driver_t;

/* 实例化 AT24C02 驱动（挂载 I2C 接口 + 初始化探测） */
at24c02_status_t at24c02_inst(bsp_at24c02_driver_t *const p_instance,
                              at24c02_i2c_interface_t *const p_i2c_instance,
                              uint8_t address_pins);

#endif /* BSP_AT24C02_DRIVER_H */
