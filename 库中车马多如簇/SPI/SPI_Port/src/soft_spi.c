#include "soft_spi.h"

#include "gpio_port.h"

static void soft_spi_delay(const st_core_software_spi_config_t *config)
{
    if (config->delay_us != NULL)
    {
        config->delay_us(config->half_period_us);
    }
}

static en_core_spi_status_t soft_spi_write(GPIO_TypeDef *port,
                                           uint16_t pin,
                                           GPIO_PinState state)
{
    if ((port == NULL) || (pin == 0U))
    {
        return CORE_SPI_OK;
    }

    return (core_gpio_write_pin_direct(port, pin, state) == CORE_GPIO_OK)
               ? CORE_SPI_OK
               : CORE_SPI_ERROR;
}

static en_core_spi_status_t soft_spi_read(GPIO_TypeDef *port,
                                          uint16_t pin,
                                          GPIO_PinState *state)
{
    if ((port == NULL) || (pin == 0U))
    {
        *state = GPIO_PIN_RESET;
        return CORE_SPI_OK;
    }

    return (core_gpio_read_pin_direct(port, pin, state) == CORE_GPIO_OK)
               ? CORE_SPI_OK
               : CORE_SPI_ERROR;
}

/**
 * @brief Transfer bytes through GPIO-driven software SPI.
 * @param[in] config Software SPI pins, mode, bit order, and delay callback.
 * @param[in] tx Transmit buffer, or NULL to transmit 0xFF.
 * @param[out] rx Receive buffer, or NULL to discard received bits.
 * @param[in] size Transfer length in bytes.
 * @return SPI transfer status.
 */
en_core_spi_status_t soft_spi_transfer(
    const st_core_software_spi_config_t *config,
    const uint8_t *tx,
    uint8_t *rx,
    uint16_t size)
{
    GPIO_PinState idle_state;
    GPIO_PinState active_state;
    uint16_t byte_index;

    if ((config == NULL) || (config->sck_port == NULL) ||
        (config->sck_pin == 0U) || (size == 0U) ||
        ((tx == NULL) && (rx == NULL)) ||
        (config->mode > CORE_SPI_MODE_3) ||
        (config->bit_order > CORE_SPI_LSB_FIRST))
    {
        return CORE_SPI_ERR_ARGUMENT;
    }

    idle_state = (config->mode >= CORE_SPI_MODE_2) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    active_state = (idle_state == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    if (soft_spi_write(config->sck_port, config->sck_pin, idle_state) != CORE_SPI_OK)
    {
        return CORE_SPI_ERROR;
    }

    for (byte_index = 0U; byte_index < size; byte_index++)
    {
        uint8_t tx_byte = (tx != NULL) ? tx[byte_index] : 0xFFU;
        uint8_t rx_byte = 0U;
        uint8_t bit_index;

        for (bit_index = 0U; bit_index < 8U; bit_index++)
        {
            uint8_t shift = (config->bit_order == CORE_SPI_MSB_FIRST)
                                ? (uint8_t)(7U - bit_index)
                                : bit_index;
            GPIO_PinState output_state = ((tx_byte >> shift) & 0x01U) != 0U
                                             ? GPIO_PIN_SET
                                             : GPIO_PIN_RESET;
            GPIO_PinState input_state;
            bool sample_on_leading = (config->mode == CORE_SPI_MODE_0) ||
                                     (config->mode == CORE_SPI_MODE_2);

            if (sample_on_leading)
            {
                if (soft_spi_write(config->mosi_port, config->mosi_pin, output_state) != CORE_SPI_OK)
                {
                    return CORE_SPI_ERROR;
                }
                soft_spi_delay(config);
                (void)soft_spi_write(config->sck_port, config->sck_pin, active_state);
                if (soft_spi_read(config->miso_port, config->miso_pin, &input_state) != CORE_SPI_OK)
                {
                    return CORE_SPI_ERROR;
                }
                soft_spi_delay(config);
                (void)soft_spi_write(config->sck_port, config->sck_pin, idle_state);
            }
            else
            {
                (void)soft_spi_write(config->sck_port, config->sck_pin, active_state);
                if (soft_spi_write(config->mosi_port, config->mosi_pin, output_state) != CORE_SPI_OK)
                {
                    return CORE_SPI_ERROR;
                }
                soft_spi_delay(config);
                (void)soft_spi_write(config->sck_port, config->sck_pin, idle_state);
                if (soft_spi_read(config->miso_port, config->miso_pin, &input_state) != CORE_SPI_OK)
                {
                    return CORE_SPI_ERROR;
                }
                soft_spi_delay(config);
            }

            if (input_state == GPIO_PIN_SET)
            {
                rx_byte |= (uint8_t)(1U << shift);
            }
        }

        if (rx != NULL)
        {
            rx[byte_index] = rx_byte;
        }
    }

    (void)soft_spi_write(config->sck_port, config->sck_pin, idle_state);
    return CORE_SPI_OK;
}
