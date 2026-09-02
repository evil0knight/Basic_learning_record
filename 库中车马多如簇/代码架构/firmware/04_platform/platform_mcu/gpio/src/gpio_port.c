#include "gpio_port.h"

#include <string.h>

typedef struct
{
    st_core_gpio_config_t config;
    bool registered;
} st_core_gpio_slot_t;

static st_core_gpio_slot_t g_gpio_slots[CORE_GPIO_PIN_MAX];

/**
 * @brief Register a physical GPIO pin under a logical pin identifier.
 * @param[in] id Logical GPIO pin identifier.
 * @param[in] config Physical pin and initialization configuration.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_register(en_core_gpio_pin_t id,
                                         const st_core_gpio_config_t *config)
{
    if ((id >= CORE_GPIO_PIN_MAX) || (config == NULL) ||
        (config->port == NULL) || (config->pin == 0U))
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }

    g_gpio_slots[id].config = *config;
    g_gpio_slots[id].registered = true;
    return CORE_GPIO_OK;
}

/**
 * @brief Apply the configured default state to all registered output pins.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_init(void)
{
    en_core_gpio_pin_t id;

    for (id = (en_core_gpio_pin_t)0; id < CORE_GPIO_PIN_MAX; id++)
    {
        const st_core_gpio_slot_t *slot = &g_gpio_slots[id];

        if (slot->registered && slot->config.initialize_output)
        {
            GPIO_PinState state = (slot->config.default_state == CORE_GPIO_PIN_SET)
                                      ? GPIO_PIN_SET
                                      : GPIO_PIN_RESET;
            (void)core_gpio_write_pin_direct(slot->config.port,
                                             slot->config.pin,
                                             state);
        }
    }

    return CORE_GPIO_OK;
}

/**
 * @brief Write a logical GPIO pin.
 * @param[in] id Logical GPIO pin identifier.
 * @param[in] state Output state.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_write_pin(en_core_gpio_pin_t id,
                                          en_core_gpio_pin_state_t state)
{
    if ((id >= CORE_GPIO_PIN_MAX) ||
        ((state != CORE_GPIO_PIN_RESET) && (state != CORE_GPIO_PIN_SET)))
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }

    if (!g_gpio_slots[id].registered)
    {
        return CORE_GPIO_ERR_NOT_CONFIGURED;
    }

    return core_gpio_write_pin_direct(g_gpio_slots[id].config.port,
                                      g_gpio_slots[id].config.pin,
                                      (state == CORE_GPIO_PIN_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Read a logical GPIO pin.
 * @param[in] id Logical GPIO pin identifier.
 * @param[out] state Pin state output.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_read_pin(en_core_gpio_pin_t id,
                                         en_core_gpio_pin_state_t *state)
{
    GPIO_PinState raw_state;
    en_core_gpio_status_t status;

    if ((id >= CORE_GPIO_PIN_MAX) || (state == NULL))
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }

    if (!g_gpio_slots[id].registered)
    {
        return CORE_GPIO_ERR_NOT_CONFIGURED;
    }

    status = core_gpio_read_pin_direct(g_gpio_slots[id].config.port,
                                       g_gpio_slots[id].config.pin,
                                       &raw_state);
    if (status == CORE_GPIO_OK)
    {
        *state = (raw_state == GPIO_PIN_SET) ? CORE_GPIO_PIN_SET : CORE_GPIO_PIN_RESET;
    }

    return status;
}

/**
 * @brief Toggle a logical GPIO output pin.
 * @param[in] id Logical GPIO pin identifier.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_toggle_pin(en_core_gpio_pin_t id)
{
    if (id >= CORE_GPIO_PIN_MAX)
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }

    if (!g_gpio_slots[id].registered)
    {
        return CORE_GPIO_ERR_NOT_CONFIGURED;
    }

    return core_gpio_toggle_pin_direct(g_gpio_slots[id].config.port,
                                       g_gpio_slots[id].config.pin);
}

/**
 * @brief Write a physical GPIO pin without logical registration.
 * @param[in] port GPIO peripheral port.
 * @param[in] pin GPIO pin mask.
 * @param[in] state Output state.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_write_pin_direct(GPIO_TypeDef *port,
                                                 uint16_t pin,
                                                 GPIO_PinState state)
{
    if ((port == NULL) || (pin == 0U))
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }

    HAL_GPIO_WritePin(port, pin, state);
    return CORE_GPIO_OK;
}

/**
 * @brief Read a physical GPIO pin without logical registration.
 * @param[in] port GPIO peripheral port.
 * @param[in] pin GPIO pin mask.
 * @param[out] state Pin state output.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_read_pin_direct(GPIO_TypeDef *port,
                                                uint16_t pin,
                                                GPIO_PinState *state)
{
    if ((port == NULL) || (pin == 0U) || (state == NULL))
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }

    *state = HAL_GPIO_ReadPin(port, pin);
    return CORE_GPIO_OK;
}

/**
 * @brief Toggle a physical GPIO pin without logical registration.
 * @param[in] port GPIO peripheral port.
 * @param[in] pin GPIO pin mask.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_toggle_pin_direct(GPIO_TypeDef *port,
                                                  uint16_t pin)
{
    if ((port == NULL) || (pin == 0U))
    {
        return CORE_GPIO_ERR_ARGUMENT;
    }

    HAL_GPIO_TogglePin(port, pin);
    return CORE_GPIO_OK;
}
