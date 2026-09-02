#ifndef I2C_PORT_H
#define I2C_PORT_H

#include <stdint.h>

#include "main.h"

typedef enum
{
    CORE_I2C_OK = 0,
    CORE_I2C_ERROR,
    CORE_I2C_ERR_ARGUMENT,
    CORE_I2C_ERR_TIMEOUT,
    CORE_I2C_ERR_BUSY,
    CORE_I2C_ERR_NOT_CONFIGURED,
    CORE_I2C_ERR_NACK
} en_core_i2c_status_t;

typedef uint8_t core_i2c_port_id_t;

#define CORE_I2C_PORT_MAX 8U

typedef enum
{
    CORE_I2C_MEM_ADDR_8BIT = 1,
    CORE_I2C_MEM_ADDR_16BIT = 2
} en_core_i2c_mem_addr_size_t;

typedef int (*core_i2c_lock_fn_t)(uint32_t timeout_ms);
typedef void (*core_i2c_unlock_fn_t)(void);

typedef struct
{
    I2C_HandleTypeDef *handle;
    core_i2c_lock_fn_t lock;
    core_i2c_unlock_fn_t unlock;
} st_core_hardware_i2c_config_t;

typedef struct
{
    GPIO_TypeDef *scl_port;
    uint16_t scl_pin;
    GPIO_TypeDef *sda_port;
    uint16_t sda_pin;
    uint32_t half_period_us;
    void (*delay_us)(uint32_t delay_us);
    core_i2c_lock_fn_t lock;
    core_i2c_unlock_fn_t unlock;
} st_core_software_i2c_config_t;

/**
 * @brief Write bytes to an I2C device.
 * @param[in] port Logical I2C slot index.
 * @param[in] device_address_7bit Seven-bit device address.
 * @param[in] data Transmit buffer.
 * @param[in] size Transmit length in bytes.
 * @param[in] timeout_ms Timeout in milliseconds.
 * @return I2C access status.
 */
en_core_i2c_status_t core_i2c_write(core_i2c_port_id_t port,
                                    uint8_t device_address_7bit,
                                    const uint8_t *data,
                                    uint16_t size,
                                    uint32_t timeout_ms);

/**
 * @brief Read bytes from an I2C device.
 * @param[in] port Logical I2C slot index.
 * @param[in] device_address_7bit Seven-bit device address.
 * @param[out] data Receive buffer.
 * @param[in] size Receive length in bytes.
 * @param[in] timeout_ms Timeout in milliseconds.
 * @return I2C access status.
 */
en_core_i2c_status_t core_i2c_read(core_i2c_port_id_t port,
                                   uint8_t device_address_7bit,
                                   uint8_t *data,
                                   uint16_t size,
                                   uint32_t timeout_ms);

/**
 * @brief Write bytes to an addressed memory location in an I2C device.
 * @param[in] port Logical I2C slot index.
 * @param[in] device_address_7bit Seven-bit device address.
 * @param[in] memory_address Device memory address.
 * @param[in] address_size Memory address width.
 * @param[in] data Transmit buffer.
 * @param[in] size Transmit length in bytes.
 * @param[in] timeout_ms Timeout in milliseconds.
 * @return I2C access status.
 */
en_core_i2c_status_t core_i2c_mem_write(core_i2c_port_id_t port,
                                        uint8_t device_address_7bit,
                                        uint16_t memory_address,
                                        en_core_i2c_mem_addr_size_t address_size,
                                        const uint8_t *data,
                                        uint16_t size,
                                        uint32_t timeout_ms);

/**
 * @brief Read bytes from an addressed memory location in an I2C device.
 * @param[in] port Logical I2C slot index.
 * @param[in] device_address_7bit Seven-bit device address.
 * @param[in] memory_address Device memory address.
 * @param[in] address_size Memory address width.
 * @param[out] data Receive buffer.
 * @param[in] size Receive length in bytes.
 * @param[in] timeout_ms Timeout in milliseconds.
 * @return I2C access status.
 */
en_core_i2c_status_t core_i2c_mem_read(core_i2c_port_id_t port,
                                       uint8_t device_address_7bit,
                                       uint16_t memory_address,
                                       en_core_i2c_mem_addr_size_t address_size,
                                       uint8_t *data,
                                       uint16_t size,
                                       uint32_t timeout_ms);

/**
 * @brief Test whether an I2C device acknowledges its address.
 * @param[in] port Logical I2C slot index.
 * @param[in] device_address_7bit Seven-bit device address.
 * @param[in] trials Maximum address attempts.
 * @param[in] timeout_ms Timeout in milliseconds per hardware attempt.
 * @return I2C access status.
 */
en_core_i2c_status_t core_i2c_is_ready(core_i2c_port_id_t port,
                                       uint8_t device_address_7bit,
                                       uint32_t trials,
                                       uint32_t timeout_ms);

#endif
