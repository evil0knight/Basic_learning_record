/**
 * @file spi.c
 * @brief STM32F4 standard peripheral library hardware SPI polling example.
 *
 * The target project initializes GPIO, clocks, SPI mode, and baud-rate division.
 * This file only operates an initialized SPI peripheral and does not control CS.
 */

#include "spi.h"

#include <stddef.h>

static SPI_TypeDef *g_spi_instance;

/**
 * @brief Attach the example to an initialized SPI peripheral.
 * @param[in] instance SPI peripheral instance.
 * @return Standard-library SPI status.
 */
en_spi_stdlib_status_t spi_stdlib_attach(SPI_TypeDef *instance)
{
    if (instance == NULL)
    {
        return SPI_STDLIB_ERR_ARGUMENT;
    }
    g_spi_instance = instance;
    return SPI_STDLIB_OK;
}

/**
 * @brief Transmit and receive one byte through hardware SPI polling.
 * @param[in] tx Byte to transmit.
 * @param[out] rx Received byte output. Must not be NULL.
 * @param[in] retry_limit Maximum polling count for each hardware flag.
 * @return Standard-library SPI status.
 */
en_spi_stdlib_status_t spi_stdlib_transfer_byte(uint8_t tx, uint8_t *rx,
                                                 uint32_t retry_limit)
{
    uint32_t retries;

    if ((rx == NULL) || (retry_limit == 0U))
    {
        return SPI_STDLIB_ERR_ARGUMENT;
    }
    if (g_spi_instance == NULL)
    {
        return SPI_STDLIB_ERR_NOT_ATTACHED;
    }

    retries = retry_limit;
    while (SPI_I2S_GetFlagStatus(g_spi_instance, SPI_I2S_FLAG_TXE) == RESET)
    {
        if (--retries == 0U) return SPI_STDLIB_ERR_TIMEOUT;
    }
    SPI_I2S_SendData(g_spi_instance, tx);

    retries = retry_limit;
    while (SPI_I2S_GetFlagStatus(g_spi_instance, SPI_I2S_FLAG_RXNE) == RESET)
    {
        if (--retries == 0U) return SPI_STDLIB_ERR_TIMEOUT;
    }
    *rx = (uint8_t)SPI_I2S_ReceiveData(g_spi_instance);
    return SPI_STDLIB_OK;
}

/**
 * @brief Transmit a byte buffer through hardware SPI polling.
 * @param[in] data Transmit buffer.
 * @param[in] size Transmit length in bytes.
 * @param[in] retry_limit Maximum polling count for each hardware flag.
 * @return Standard-library SPI status.
 */
en_spi_stdlib_status_t spi_stdlib_write(const uint8_t *data, uint16_t size,
                                         uint32_t retry_limit)
{
    uint16_t index;

    if ((data == NULL) || (size == 0U)) return SPI_STDLIB_ERR_ARGUMENT;
    for (index = 0U; index < size; index++)
    {
        uint8_t discarded;
        en_spi_stdlib_status_t status =
            spi_stdlib_transfer_byte(data[index], &discarded, retry_limit);
        if (status != SPI_STDLIB_OK) return status;
    }
    return SPI_STDLIB_OK;
}

/**
 * @brief Receive a byte buffer by transmitting 0xFF dummy bytes.
 * @param[out] data Receive buffer.
 * @param[in] size Receive length in bytes.
 * @param[in] retry_limit Maximum polling count for each hardware flag.
 * @return Standard-library SPI status.
 */
en_spi_stdlib_status_t spi_stdlib_read(uint8_t *data, uint16_t size,
                                        uint32_t retry_limit)
{
    uint16_t index;

    if ((data == NULL) || (size == 0U)) return SPI_STDLIB_ERR_ARGUMENT;
    for (index = 0U; index < size; index++)
    {
        en_spi_stdlib_status_t status =
            spi_stdlib_transfer_byte(0xFFU, &data[index], retry_limit);
        if (status != SPI_STDLIB_OK) return status;
    }
    return SPI_STDLIB_OK;
}
