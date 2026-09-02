#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>

#include "spi_port.h"

typedef enum
{
    ST7789_OK = 0,
    ST7789_ERROR,
    ST7789_ERR_ARGUMENT,
    ST7789_ERR_NOT_READY
} st7789_status_t;

typedef struct
{
    uint16_t width;
    uint16_t height;
    core_spi_port_id_t spi_port;
    void (*delay_ms)(uint32_t delay_ms);
    uint8_t initialized;
} st7789_t;

st7789_status_t st7789_init(st7789_t *display);
st7789_status_t st7789_set_window(st7789_t *display,
                                  uint16_t x,
                                  uint16_t y,
                                  uint16_t width,
                                  uint16_t height);
st7789_status_t st7789_fill(st7789_t *display, uint16_t color);
st7789_status_t st7789_draw_image(st7789_t *display,
                                  uint16_t x,
                                  uint16_t y,
                                  uint16_t width,
                                  uint16_t height,
                                  const uint16_t *pixels);
st7789_status_t st7789_sleep(st7789_t *display);
st7789_status_t st7789_wakeup(st7789_t *display);

#endif
