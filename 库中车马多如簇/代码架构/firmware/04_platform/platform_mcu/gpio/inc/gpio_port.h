#ifndef GPIO_PORT_H
#define GPIO_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "main.h"

typedef enum
{
    CORE_GPIO_OK = 0,
    CORE_GPIO_ERROR,
    CORE_GPIO_ERR_ARGUMENT,
    CORE_GPIO_ERR_NOT_CONFIGURED
} en_core_gpio_status_t;

typedef enum
{
    CORE_GPIO_PIN_RESET = 0,
    CORE_GPIO_PIN_SET
} en_core_gpio_pin_state_t;

typedef enum
{
    CORE_GPIO_DISPLAY_RESET = 0,
    CORE_GPIO_DISPLAY_DC,
    CORE_GPIO_DISPLAY_BACKLIGHT,
    CORE_GPIO_FLASH_WP,
    CORE_GPIO_LED_EN,
    CORE_GPIO_KEY1,
    CORE_GPIO_SENSOR_INT,
    CORE_GPIO_PIN_MAX
} en_core_gpio_pin_t;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    en_core_gpio_pin_state_t default_state;
    bool initialize_output;
} st_core_gpio_config_t;

/**
 * @brief Register a physical GPIO pin under a logical pin identifier.
 * @param[in] id Logical GPIO pin identifier.
 * @param[in] config Physical pin and initialization configuration.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_register(en_core_gpio_pin_t id,
                                         const st_core_gpio_config_t *config);

/**
 * @brief Apply the configured default state to all registered output pins.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_init(void);

/**
 * @brief Write a logical GPIO pin.
 * @param[in] id Logical GPIO pin identifier.
 * @param[in] state Output state.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_write_pin(en_core_gpio_pin_t id,
                                          en_core_gpio_pin_state_t state);

/**
 * @brief Read a logical GPIO pin.
 * @param[in] id Logical GPIO pin identifier.
 * @param[out] state Pin state output.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_read_pin(en_core_gpio_pin_t id,
                                         en_core_gpio_pin_state_t *state);

/**
 * @brief Toggle a logical GPIO output pin.
 * @param[in] id Logical GPIO pin identifier.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_toggle_pin(en_core_gpio_pin_t id);

/**
 * @brief Write a physical GPIO pin without logical registration.
 * @param[in] port GPIO peripheral port.
 * @param[in] pin GPIO pin mask.
 * @param[in] state Output state.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_write_pin_direct(GPIO_TypeDef *port,
                                                 uint16_t pin,
                                                 GPIO_PinState state);

/**
 * @brief Read a physical GPIO pin without logical registration.
 * @param[in] port GPIO peripheral port.
 * @param[in] pin GPIO pin mask.
 * @param[out] state Pin state output.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_read_pin_direct(GPIO_TypeDef *port,
                                                uint16_t pin,
                                                GPIO_PinState *state);

/**
 * @brief Toggle a physical GPIO pin without logical registration.
 * @param[in] port GPIO peripheral port.
 * @param[in] pin GPIO pin mask.
 * @return GPIO access status.
 */
en_core_gpio_status_t core_gpio_toggle_pin_direct(GPIO_TypeDef *port,
                                                  uint16_t pin);

#endif
