#include "soft_i2c.h"

#include "gpio_port.h"

static void soft_i2c_delay(const st_core_software_i2c_config_t *config)
{
    config->delay_us(config->half_period_us);
}

static en_core_i2c_status_t soft_i2c_set(GPIO_TypeDef *port,
                                         uint16_t pin,
                                         GPIO_PinState state)
{
    return (core_gpio_write_pin_direct(port, pin, state) == CORE_GPIO_OK)
               ? CORE_I2C_OK
               : CORE_I2C_ERROR;
}

static en_core_i2c_status_t soft_i2c_get(GPIO_TypeDef *port,
                                         uint16_t pin,
                                         GPIO_PinState *state)
{
    return (core_gpio_read_pin_direct(port, pin, state) == CORE_GPIO_OK)
               ? CORE_I2C_OK
               : CORE_I2C_ERROR;
}

static en_core_i2c_status_t soft_i2c_start(const st_core_software_i2c_config_t *config)
{
    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_SET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_RESET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    return soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_RESET);
}

static en_core_i2c_status_t soft_i2c_stop(const st_core_software_i2c_config_t *config)
{
    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_RESET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_SET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    return CORE_I2C_OK;
}

static en_core_i2c_status_t soft_i2c_finish(
    const st_core_software_i2c_config_t *config,
    en_core_i2c_status_t status)
{
    en_core_i2c_status_t stop_status = soft_i2c_stop(config);

    return (stop_status == CORE_I2C_OK) ? status : stop_status;
}

static en_core_i2c_status_t soft_i2c_send_byte(
    const st_core_software_i2c_config_t *config,
    uint8_t data)
{
    uint8_t bit;
    GPIO_PinState ack_state;

    for (bit = 0U; bit < 8U; bit++)
    {
        GPIO_PinState state = ((data & 0x80U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;

        if (soft_i2c_set(config->sda_port, config->sda_pin, state) != CORE_I2C_OK)
            return CORE_I2C_ERROR;
        data <<= 1U;
        soft_i2c_delay(config);
        if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET) != CORE_I2C_OK)
            return CORE_I2C_ERROR;
        soft_i2c_delay(config);
        if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_RESET) != CORE_I2C_OK)
            return CORE_I2C_ERROR;
    }

    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_SET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_get(config->sda_port, config->sda_pin, &ack_state) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_RESET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;

    return (ack_state == GPIO_PIN_RESET) ? CORE_I2C_OK : CORE_I2C_ERR_NACK;
}

static en_core_i2c_status_t soft_i2c_receive_byte(
    const st_core_software_i2c_config_t *config,
    uint8_t *data,
    int acknowledge)
{
    uint8_t bit;

    *data = 0U;
    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_SET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;

    for (bit = 0U; bit < 8U; bit++)
    {
        GPIO_PinState state;

        *data <<= 1U;
        soft_i2c_delay(config);
        if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET) != CORE_I2C_OK)
            return CORE_I2C_ERROR;
        soft_i2c_delay(config);
        if (soft_i2c_get(config->sda_port, config->sda_pin, &state) != CORE_I2C_OK)
            return CORE_I2C_ERROR;
        if (state == GPIO_PIN_SET)
        {
            *data |= 1U;
        }
        if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_RESET) != CORE_I2C_OK)
            return CORE_I2C_ERROR;
    }

    if (soft_i2c_set(config->sda_port,
                     config->sda_pin,
                     acknowledge ? GPIO_PIN_RESET : GPIO_PIN_SET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_RESET) != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    return soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_SET);
}

static en_core_i2c_status_t soft_i2c_send_memory_address(
    const st_core_software_i2c_config_t *config,
    uint16_t memory_address,
    en_core_i2c_mem_addr_size_t address_size)
{
    en_core_i2c_status_t status;

    if (address_size == CORE_I2C_MEM_ADDR_16BIT)
    {
        status = soft_i2c_send_byte(config, (uint8_t)(memory_address >> 8U));
        if (status != CORE_I2C_OK)
        {
            return status;
        }
    }

    return soft_i2c_send_byte(config, (uint8_t)memory_address);
}

en_core_i2c_status_t soft_i2c_write(const st_core_software_i2c_config_t *config,
                                    uint8_t device_address_7bit,
                                    const uint8_t *data,
                                    uint16_t size)
{
    uint16_t index;
    en_core_i2c_status_t status = soft_i2c_start(config);

    if (status == CORE_I2C_OK)
        status = soft_i2c_send_byte(config, (uint8_t)(device_address_7bit << 1U));
    for (index = 0U; (index < size) && (status == CORE_I2C_OK); index++)
        status = soft_i2c_send_byte(config, data[index]);

    return soft_i2c_finish(config, status);
}

en_core_i2c_status_t soft_i2c_read(const st_core_software_i2c_config_t *config,
                                   uint8_t device_address_7bit,
                                   uint8_t *data,
                                   uint16_t size)
{
    uint16_t index;
    en_core_i2c_status_t status = soft_i2c_start(config);

    if (status == CORE_I2C_OK)
        status = soft_i2c_send_byte(config,
                                    (uint8_t)((device_address_7bit << 1U) | 1U));
    for (index = 0U; (index < size) && (status == CORE_I2C_OK); index++)
        status = soft_i2c_receive_byte(config, &data[index], index + 1U < size);

    return soft_i2c_finish(config, status);
}

en_core_i2c_status_t soft_i2c_mem_write(const st_core_software_i2c_config_t *config,
                                        uint8_t device_address_7bit,
                                        uint16_t memory_address,
                                        en_core_i2c_mem_addr_size_t address_size,
                                        const uint8_t *data,
                                        uint16_t size)
{
    uint16_t index;
    en_core_i2c_status_t status = soft_i2c_start(config);

    if (status == CORE_I2C_OK)
        status = soft_i2c_send_byte(config, (uint8_t)(device_address_7bit << 1U));
    if (status == CORE_I2C_OK)
        status = soft_i2c_send_memory_address(config, memory_address, address_size);
    for (index = 0U; (index < size) && (status == CORE_I2C_OK); index++)
        status = soft_i2c_send_byte(config, data[index]);

    return soft_i2c_finish(config, status);
}

en_core_i2c_status_t soft_i2c_mem_read(const st_core_software_i2c_config_t *config,
                                       uint8_t device_address_7bit,
                                       uint16_t memory_address,
                                       en_core_i2c_mem_addr_size_t address_size,
                                       uint8_t *data,
                                       uint16_t size)
{
    uint16_t index;
    en_core_i2c_status_t status = soft_i2c_start(config);

    if (status == CORE_I2C_OK)
        status = soft_i2c_send_byte(config, (uint8_t)(device_address_7bit << 1U));
    if (status == CORE_I2C_OK)
        status = soft_i2c_send_memory_address(config, memory_address, address_size);
    if (status == CORE_I2C_OK)
        status = soft_i2c_start(config);
    if (status == CORE_I2C_OK)
        status = soft_i2c_send_byte(config,
                                    (uint8_t)((device_address_7bit << 1U) | 1U));
    for (index = 0U; (index < size) && (status == CORE_I2C_OK); index++)
        status = soft_i2c_receive_byte(config, &data[index], index + 1U < size);

    return soft_i2c_finish(config, status);
}

en_core_i2c_status_t soft_i2c_is_ready(const st_core_software_i2c_config_t *config,
                                       uint8_t device_address_7bit,
                                       uint32_t trials)
{
    uint32_t trial;

    for (trial = 0U; trial < trials; trial++)
    {
        en_core_i2c_status_t status = soft_i2c_start(config);

        if (status == CORE_I2C_OK)
            status = soft_i2c_send_byte(config, (uint8_t)(device_address_7bit << 1U));
        status = soft_i2c_finish(config, status);
        if (status == CORE_I2C_OK)
        {
            return CORE_I2C_OK;
        }
        if (status != CORE_I2C_ERR_NACK)
        {
            return status;
        }
    }

    return CORE_I2C_ERR_NACK;
}
