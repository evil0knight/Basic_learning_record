#include "software_spi.h"

#include <stddef.h>

/**
 * @brief Transfer bytes through a GPIO-driven software SPI bus.
 * @param[in] spi Software SPI callbacks, mode, and bit order.
 * @param[in] tx Transmit buffer, or NULL to transmit 0xFF dummy bytes.
 * @param[out] rx Receive buffer, or NULL to discard received bytes.
 * @param[in] size Transfer length in bytes.
 * @return Software SPI status.
 */
en_software_spi_status_t software_spi_transfer(const software_spi_t *spi,
                                                const uint8_t *tx,
                                                uint8_t *rx,
                                                uint16_t size)
{
    uint8_t idle;
    uint8_t active;
    uint16_t byte_index;

    if ((spi == NULL) || (spi->write_sck == NULL) ||
        (spi->delay_half_period == NULL) || (size == 0U) ||
        ((tx == NULL) && (rx == NULL)) || (spi->mode > SOFTWARE_SPI_MODE_3))
    {
        return SOFTWARE_SPI_ERR_ARGUMENT;
    }

    idle = (spi->mode >= SOFTWARE_SPI_MODE_2) ? 1U : 0U;
    active = (uint8_t)!idle;
    spi->write_sck(idle);

    for (byte_index = 0U; byte_index < size; byte_index++)
    {
        uint8_t tx_byte = (tx != NULL) ? tx[byte_index] : 0xFFU;
        uint8_t rx_byte = 0U;
        uint8_t bit_index;

        for (bit_index = 0U; bit_index < 8U; bit_index++)
        {
            uint8_t shift = (spi->lsb_first != 0U) ? bit_index : (uint8_t)(7U - bit_index);
            uint8_t bit = (uint8_t)((tx_byte >> shift) & 0x01U);
            uint8_t leading = (spi->mode == SOFTWARE_SPI_MODE_0) ||
                              (spi->mode == SOFTWARE_SPI_MODE_2);

            if (leading != 0U)
            {
                if (spi->write_mosi != NULL) spi->write_mosi(bit);
                spi->delay_half_period();
                spi->write_sck(active);
                if ((spi->read_miso != NULL) && (spi->read_miso() != 0U))
                    rx_byte |= (uint8_t)(1U << shift);
                spi->delay_half_period();
                spi->write_sck(idle);
            }
            else
            {
                spi->write_sck(active);
                if (spi->write_mosi != NULL) spi->write_mosi(bit);
                spi->delay_half_period();
                spi->write_sck(idle);
                if ((spi->read_miso != NULL) && (spi->read_miso() != 0U))
                    rx_byte |= (uint8_t)(1U << shift);
                spi->delay_half_period();
            }
        }

        if (rx != NULL) rx[byte_index] = rx_byte;
    }

    spi->write_sck(idle);
    return SOFTWARE_SPI_OK;
}
