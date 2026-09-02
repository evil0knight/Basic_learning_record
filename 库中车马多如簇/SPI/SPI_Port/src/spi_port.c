#include "spi_port.h"

#include <stdbool.h>

#include "gpio_port.h"
#include "soft_spi.h"

typedef enum
{
    CORE_SPI_TYPE_NONE = 0,
    CORE_SPI_TYPE_HARDWARE,
    CORE_SPI_TYPE_SOFTWARE
} en_core_spi_type_t;

typedef struct
{
    bool transaction_active;
    en_core_spi_type_t type;
    union
    {
        st_core_hardware_spi_config_t hardware;
        st_core_software_spi_config_t software;
    } config;
} st_core_spi_slot_t;

#include "spi_port_config.h"

static en_core_spi_status_t core_spi_validate_common(core_spi_port_id_t port)
{
    if (port >= CORE_SPI_PORT_MAX)
    {
        return CORE_SPI_ERR_ARGUMENT;
    }

    if (g_spi_slots[port].type == CORE_SPI_TYPE_NONE)
    {
        return CORE_SPI_ERR_NOT_CONFIGURED;
    }

    return CORE_SPI_OK;
}

static en_core_spi_status_t core_spi_from_hal(HAL_StatusTypeDef status)
{
    switch (status)
    {
        case HAL_OK:
            return CORE_SPI_OK;
        case HAL_BUSY:
            return CORE_SPI_ERR_BUSY;
        case HAL_TIMEOUT:
            return CORE_SPI_ERR_TIMEOUT;
        default:
            return CORE_SPI_ERROR;
    }
}

/**
 * @brief Lock the selected SPI connection and assert its chip-select pin.
 * @param[in] port Logical SPI slot index.
 * @return SPI transaction status.
 * @note Every successful begin must be paired with core_spi_end().
 */
en_core_spi_status_t core_spi_begin(core_spi_port_id_t port)
{
    st_core_spi_slot_t *slot;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    GPIO_PinState active_state;
    core_spi_lock_fn_t lock;

    en_core_spi_status_t validation = core_spi_validate_common(port);

    if (validation != CORE_SPI_OK)
    {
        return validation;
    }

    slot = &g_spi_slots[port];
    if (slot->transaction_active)
    {
        return CORE_SPI_ERR_BUSY;
    }

    if (slot->type == CORE_SPI_TYPE_SOFTWARE)
    {
        cs_port = slot->config.software.cs_port;
        cs_pin = slot->config.software.cs_pin;
        active_state = slot->config.software.cs_active_state;
        lock = slot->config.software.lock;
    }
    else
    {
        cs_port = slot->config.hardware.cs_port;
        cs_pin = slot->config.hardware.cs_pin;
        active_state = slot->config.hardware.cs_active_state;
        lock = slot->config.hardware.lock;
    }

    if ((lock != NULL) && (lock(HAL_MAX_DELAY) != 0))
    {
        return CORE_SPI_ERR_BUSY;
    }

    if (core_gpio_write_pin_direct(cs_port, cs_pin, active_state) != CORE_GPIO_OK)
    {
        if ((slot->type == CORE_SPI_TYPE_SOFTWARE) &&
            (slot->config.software.unlock != NULL))
        {
            slot->config.software.unlock();
        }
        else if ((slot->type == CORE_SPI_TYPE_HARDWARE) &&
                 (slot->config.hardware.unlock != NULL))
        {
            slot->config.hardware.unlock();
        }
        return CORE_SPI_ERROR;
    }

    slot->transaction_active = true;
    return CORE_SPI_OK;
}

/**
 * @brief Transfer data while the selected SPI transaction is active.
 * @param[in] port Logical SPI slot index.
 * @param[in] tx Transmit buffer, or NULL to transmit 0xFF while reading.
 * @param[out] rx Receive buffer, or NULL to discard received bytes.
 * @param[in] size Transfer length in bytes.
 * @param[in] timeout_ms Hardware SPI timeout in milliseconds.
 * @return SPI transfer status.
 * @note core_spi_begin() must succeed before calling this function.
 */
en_core_spi_status_t core_spi_transfer(core_spi_port_id_t port,
                                       const uint8_t *tx,
                                       uint8_t *rx,
                                       uint16_t size,
                                       uint32_t timeout_ms)
{
    st_core_spi_slot_t *slot;

    if ((size == 0U) || ((tx == NULL) && (rx == NULL)))
    {
        return CORE_SPI_ERR_ARGUMENT;
    }

    en_core_spi_status_t validation = core_spi_validate_common(port);

    if (validation != CORE_SPI_OK)
    {
        return validation;
    }

    slot = &g_spi_slots[port];
    if (!slot->transaction_active)
    {
        return CORE_SPI_ERR_STATE;
    }

    if (slot->type == CORE_SPI_TYPE_SOFTWARE)
    {
        return soft_spi_transfer(&slot->config.software, tx, rx, size);
    }

    if ((tx != NULL) && (rx == NULL))
    {
        return core_spi_from_hal(HAL_SPI_Transmit(slot->config.hardware.handle,
                                                  (uint8_t *)tx,
                                                  size,
                                                  timeout_ms));
    }

    if ((tx != NULL) && (rx != NULL))
    {
        return core_spi_from_hal(HAL_SPI_TransmitReceive(slot->config.hardware.handle,
                                                         (uint8_t *)tx,
                                                         rx,
                                                         size,
                                                         timeout_ms));
    }

    for (uint16_t index = 0U; index < size; index++)
    {
        uint8_t dummy = 0xFFU;
        en_core_spi_status_t status = core_spi_from_hal(
            HAL_SPI_TransmitReceive(slot->config.hardware.handle,
                                    &dummy,
                                    &rx[index],
                                    1U,
                                    timeout_ms));
        if (status != CORE_SPI_OK)
        {
            return status;
        }
    }

    return CORE_SPI_OK;
}

/**
 * @brief Write data while the selected SPI transaction is active.
 * @param[in] port Logical SPI slot index.
 * @param[in] data Transmit buffer.
 * @param[in] size Transmit length in bytes.
 * @param[in] timeout_ms Hardware SPI timeout in milliseconds.
 * @return SPI transfer status.
 */
en_core_spi_status_t core_spi_write(core_spi_port_id_t port,
                                    const uint8_t *data,
                                    uint16_t size,
                                    uint32_t timeout_ms)
{
    return core_spi_transfer(port, data, NULL, size, timeout_ms);
}

/**
 * @brief Read data while generating SPI clock with 0xFF dummy bytes.
 * @param[in] port Logical SPI slot index.
 * @param[out] data Receive buffer.
 * @param[in] size Receive length in bytes.
 * @param[in] timeout_ms Hardware SPI timeout in milliseconds.
 * @return SPI transfer status.
 */
en_core_spi_status_t core_spi_read(core_spi_port_id_t port,
                                   uint8_t *data,
                                   uint16_t size,
                                   uint32_t timeout_ms)
{
    return core_spi_transfer(port, NULL, data, size, timeout_ms);
}

/**
 * @brief Release chip select and unlock the selected SPI connection.
 * @param[in] port Logical SPI slot index.
 * @return SPI transaction cleanup status.
 */
en_core_spi_status_t core_spi_end(core_spi_port_id_t port)
{
    st_core_spi_slot_t *slot;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    GPIO_PinState inactive_state;
    core_spi_unlock_fn_t unlock;

    en_core_spi_status_t validation = core_spi_validate_common(port);

    if (validation != CORE_SPI_OK)
    {
        return validation;
    }

    slot = &g_spi_slots[port];
    if (!slot->transaction_active)
    {
        return CORE_SPI_ERR_STATE;
    }

    if (slot->type == CORE_SPI_TYPE_SOFTWARE)
    {
        cs_port = slot->config.software.cs_port;
        cs_pin = slot->config.software.cs_pin;
        inactive_state = (slot->config.software.cs_active_state == GPIO_PIN_SET)
                             ? GPIO_PIN_RESET
                             : GPIO_PIN_SET;
        unlock = slot->config.software.unlock;
    }
    else
    {
        cs_port = slot->config.hardware.cs_port;
        cs_pin = slot->config.hardware.cs_pin;
        inactive_state = (slot->config.hardware.cs_active_state == GPIO_PIN_SET)
                             ? GPIO_PIN_RESET
                             : GPIO_PIN_SET;
        unlock = slot->config.hardware.unlock;
    }

    slot->transaction_active = false;
    if (core_gpio_write_pin_direct(cs_port, cs_pin, inactive_state) != CORE_GPIO_OK)
    {
        if (unlock != NULL)
        {
            unlock();
        }
        return CORE_SPI_ERROR;
    }

    if (unlock != NULL)
    {
        unlock();
    }
    return CORE_SPI_OK;
}
