#ifndef SOFTWARE_SPI_H
#define SOFTWARE_SPI_H

#include <stdint.h>

typedef enum
{
    SOFTWARE_SPI_MODE_0 = 0,
    SOFTWARE_SPI_MODE_1,
    SOFTWARE_SPI_MODE_2,
    SOFTWARE_SPI_MODE_3
} software_spi_mode_t;

typedef enum
{
    SOFTWARE_SPI_OK = 0,
    SOFTWARE_SPI_ERR_ARGUMENT
} en_software_spi_status_t;

typedef struct
{
    void (*write_sck)(uint8_t level);
    void (*write_mosi)(uint8_t level);
    uint8_t (*read_miso)(void);
    void (*delay_half_period)(void);
    software_spi_mode_t mode;
    uint8_t lsb_first;
} software_spi_t;

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
                                                uint16_t size);

#endif
