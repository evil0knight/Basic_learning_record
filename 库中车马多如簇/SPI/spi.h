#ifndef SPI_H
#define SPI_H

#include <stdint.h>

#include "stm32f4xx_spi.h"

typedef enum
{
    SPI_STDLIB_OK = 0,
    SPI_STDLIB_ERR_ARGUMENT,
    SPI_STDLIB_ERR_TIMEOUT,
    SPI_STDLIB_ERR_NOT_ATTACHED
} en_spi_stdlib_status_t;

/**
 * @brief Attach the standard-library SPI example to an initialized peripheral.
 * @param[in] instance SPI peripheral instance.
 * @return Standard-library SPI status.
 */
en_spi_stdlib_status_t spi_stdlib_attach(SPI_TypeDef *instance);

/**
 * @brief Transmit and receive one byte through hardware SPI polling.
 * @param[in] tx Byte to transmit.
 * @param[out] rx Received byte output. Must not be NULL.
 * @param[in] retry_limit Maximum polling count for each hardware flag.
 * @return Standard-library SPI status.
 */
en_spi_stdlib_status_t spi_stdlib_transfer_byte(uint8_t tx, uint8_t *rx,
                                                 uint32_t retry_limit);

/**
 * @brief Transmit a byte buffer through hardware SPI polling.
 * @param[in] data Transmit buffer.
 * @param[in] size Transmit length in bytes.
 * @param[in] retry_limit Maximum polling count for each hardware flag.
 * @return Standard-library SPI status.
 */
en_spi_stdlib_status_t spi_stdlib_write(const uint8_t *data, uint16_t size,
                                         uint32_t retry_limit);

/**
 * @brief Receive a byte buffer by transmitting 0xFF dummy bytes.
 * @param[out] data Receive buffer.
 * @param[in] size Receive length in bytes.
 * @param[in] retry_limit Maximum polling count for each hardware flag.
 * @return Standard-library SPI status.
 */
en_spi_stdlib_status_t spi_stdlib_read(uint8_t *data, uint16_t size,
                                        uint32_t retry_limit);

#endif
