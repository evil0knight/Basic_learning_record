#ifndef SOFT_SPI_H
#define SOFT_SPI_H

#include "spi_port.h"

/**
 * @brief Transfer bytes through a configured GPIO-driven software SPI bus.
 * @param[in] config Software SPI pins, mode, bit order, and delay callback.
 * @param[in] tx Transmit buffer, or NULL to transmit 0xFF dummy bytes.
 * @param[out] rx Receive buffer, or NULL to discard received bytes.
 * @param[in] size Transfer length in bytes.
 * @return SPI transfer status.
 */
en_core_spi_status_t soft_spi_transfer(
    const st_core_software_spi_config_t *config,
    const uint8_t *tx,
    uint8_t *rx,
    uint16_t size);

#endif
