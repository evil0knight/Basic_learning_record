/****************************************************************************************
 * @file bsp_wrapper_led.c
 * @brief Wrapper implementation for platform-mounted LED drivers.
 ****************************************************************************************/
#include "bsp_wrapper_led.h"

#define LED_WRAPPER_MAX_INSTANCE_NUMBER (10U)

static led_wrapper_driver_t g_led_wrapper_driver;
static bool g_led_wrapper_registered = false;

bool drv_wrapper_led_register(const led_wrapper_driver_t * const driver)
{
    if ((driver == NULL) || (driver->init == NULL) || (driver->control == NULL) ||
        g_led_wrapper_registered) {
        return false;
    }

    g_led_wrapper_driver = *driver;
    g_led_wrapper_registered = true;
    return true;
}

void drv_wrapper_led_init(void)
{
    if (g_led_wrapper_registered) {
        g_led_wrapper_driver.init(&g_led_wrapper_driver);
    }
}

void drv_wrapper_led_control(uint32_t led_index,
                             uint32_t cycle_time_ms,
                             uint32_t blink_time_ms,
                             led_wrapper_proportion_t proportion)
{
    if (!g_led_wrapper_registered || (led_index >= LED_WRAPPER_MAX_INSTANCE_NUMBER) ||
        (proportion > LED_WRAPPER_PROPORTION_1_3)) {
        return;
    }

    g_led_wrapper_driver.control(&g_led_wrapper_driver,
                                 led_index,
                                 cycle_time_ms,
                                 blink_time_ms,
                                 proportion);
}
