/****************************************************************************************
 * @file bsp_wrapper_led.h
 * @brief Stable LED API exposed to the application layer.
 ****************************************************************************************/
#ifndef BSP_WRAPPER_LED_H
#define BSP_WRAPPER_LED_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    LED_WRAPPER_PROPORTION_1_1 = 0,
    LED_WRAPPER_PROPORTION_1_2 = 1,
    LED_WRAPPER_PROPORTION_1_3 = 2,
} led_wrapper_proportion_t;

typedef struct led_wrapper_driver_t led_wrapper_driver_t;

typedef void (*pf_led_wrapper_init)(led_wrapper_driver_t * const self);
typedef void (*pf_led_wrapper_control)(led_wrapper_driver_t * const self,
                                       uint32_t led_index,
                                       uint32_t cycle_time_ms,
                                       uint32_t blink_time_ms,
                                       led_wrapper_proportion_t proportion);

struct led_wrapper_driver_t {
    void *user_data;
    pf_led_wrapper_init init;
    pf_led_wrapper_control control;
};

/* Platform-only registration entry. APP should use the APIs below. */
bool drv_wrapper_led_register(const led_wrapper_driver_t * const driver);

void drv_wrapper_led_init(void);

void drv_wrapper_led_control(uint32_t led_index,
                             uint32_t cycle_time_ms,
                             uint32_t blink_time_ms,
                             led_wrapper_proportion_t proportion);

#endif
