#ifndef SPI_PORT_H
#define SPI_PORT_H

#include <stdint.h>

#include "main.h"

typedef enum
{
    CORE_SPI_OK = 0,
    CORE_SPI_ERROR,
    CORE_SPI_ERR_ARGUMENT,
    CORE_SPI_ERR_TIMEOUT,
    CORE_SPI_ERR_BUSY,
    CORE_SPI_ERR_STATE,
    CORE_SPI_ERR_NOT_CONFIGURED
} en_core_spi_status_t;

typedef uint8_t core_spi_port_id_t;

#define CORE_SPI_PORT_MAX 8U

typedef enum
{
    CORE_SPI_MODE_0 = 0,
    CORE_SPI_MODE_1,
    CORE_SPI_MODE_2,
    CORE_SPI_MODE_3
} en_core_spi_mode_t;

typedef enum
{
    CORE_SPI_MSB_FIRST = 0,
    CORE_SPI_LSB_FIRST
} en_core_spi_bit_order_t;

typedef int (*core_spi_lock_fn_t)(uint32_t timeout_ms);
typedef void (*core_spi_unlock_fn_t)(void);

typedef struct
{
    SPI_HandleTypeDef *handle;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    GPIO_PinState cs_active_state;
    core_spi_lock_fn_t lock;
    core_spi_unlock_fn_t unlock;
} st_core_hardware_spi_config_t;

typedef struct
{
    GPIO_TypeDef *sck_port;
    uint16_t sck_pin;
    GPIO_TypeDef *mosi_port;
    uint16_t mosi_pin;
    GPIO_TypeDef *miso_port;
    uint16_t miso_pin;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    GPIO_PinState cs_active_state;
    en_core_spi_mode_t mode;
    en_core_spi_bit_order_t bit_order;
    uint32_t half_period_us;
    void (*delay_us)(uint32_t delay_us);
    core_spi_lock_fn_t lock;
    core_spi_unlock_fn_t unlock;
} st_core_software_spi_config_t;

/**
 * @brief Lock an SPI bus and activate its chip-select pin.
 * @param[in] port Logical SPI slot index.
 * @return SPI access status.
 */
en_core_spi_status_t core_spi_begin(core_spi_port_id_t port);

/**
 * @brief Transmit and receive data while holding the SPI chip-select pin active.
 * @param[in] port Logical SPI slot index.
 * @param[in] tx Transmit buffer, or NULL to transmit dummy bytes.
 * @param[out] rx Receive buffer, or NULL to discard received bytes.
 * @param[in] size Transfer length in bytes.
 * @param[in] timeout_ms Timeout in milliseconds.
 * @return SPI access status.
 */
en_core_spi_status_t core_spi_transfer(core_spi_port_id_t port,
                                       const uint8_t *tx,
                                       uint8_t *rx,
                                       uint16_t size,
                                       uint32_t timeout_ms);

/**
 * @brief Transmit data while holding the SPI chip-select pin active.
 * @param[in] port Logical SPI slot index.
 * @param[in] data Transmit buffer.
 * @param[in] size Transmit length in bytes.
 * @param[in] timeout_ms Timeout in milliseconds.
 * @return SPI access status.
 */
en_core_spi_status_t core_spi_write(core_spi_port_id_t port,
                                    const uint8_t *data,
                                    uint16_t size,
                                    uint32_t timeout_ms);

/**
 * @brief Receive data while holding the SPI chip-select pin active.
 * @param[in] port Logical SPI slot index.
 * @param[out] data Receive buffer.
 * @param[in] size Receive length in bytes.
 * @param[in] timeout_ms Timeout in milliseconds.
 * @return SPI access status.
 */
en_core_spi_status_t core_spi_read(core_spi_port_id_t port,
                                   uint8_t *data,
                                   uint16_t size,
                                   uint32_t timeout_ms);

/**
 * @brief Deactivate the SPI chip-select pin and unlock the bus.
 * @param[in] port Logical SPI slot index.
 * @return SPI access status.
 */
en_core_spi_status_t core_spi_end(core_spi_port_id_t port);

#endif
