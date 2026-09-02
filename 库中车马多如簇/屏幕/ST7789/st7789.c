#include "st7789.h"

#include <stddef.h>

#include "gpio_port.h"
#include "spi_port.h"

#define ST7789_SWRESET   0x01U
#define ST7789_SLPOUT    0x11U
#define ST7789_COLMOD    0x3AU
#define ST7789_MADCTL    0x36U
#define ST7789_CASET     0x2AU
#define ST7789_RASET     0x2BU
#define ST7789_RAMWR     0x2CU
#define ST7789_DISPON    0x29U
#define ST7789_SLPIN     0x10U
#define ST7789_SPI_TIMEOUT_MS 1000UL

static st7789_status_t st7789_write(st7789_t *display,
                                    uint8_t data_mode,
                                    const uint8_t *data,
                                    uint16_t size)
{
    en_core_spi_status_t status;

    if ((display == NULL) || (data == NULL) || (size == 0U))
    {
        return ST7789_ERR_ARGUMENT;
    }

    if (core_gpio_write_pin(CORE_GPIO_DISPLAY_DC,
                            data_mode != 0U ? CORE_GPIO_PIN_SET : CORE_GPIO_PIN_RESET) != CORE_GPIO_OK)
    {
        return ST7789_ERROR;
    }

    status = core_spi_begin(display->spi_port);
    if (status != CORE_SPI_OK)
    {
        return ST7789_ERROR;
    }
    status = core_spi_write(display->spi_port,
                            data,
                            size,
                            ST7789_SPI_TIMEOUT_MS);
    if (core_spi_end(display->spi_port) != CORE_SPI_OK)
    {
        return ST7789_ERROR;
    }

    return (status == CORE_SPI_OK) ? ST7789_OK : ST7789_ERROR;
}

static st7789_status_t st7789_command(st7789_t *display, uint8_t command)
{
    return st7789_write(display, 0U, &command, 1U);
}

static st7789_status_t st7789_data(st7789_t *display,
                                   const uint8_t *data,
                                   uint16_t size)
{
    return st7789_write(display, 1U, data, size);
}

static st7789_status_t st7789_reset(st7789_t *display)
{
    if ((display == NULL) || (display->delay_ms == NULL))
    {
        return ST7789_ERR_ARGUMENT;
    }

    if (core_gpio_write_pin(CORE_GPIO_DISPLAY_RESET, CORE_GPIO_PIN_RESET) != CORE_GPIO_OK)
    {
        return ST7789_ERROR;
    }
    display->delay_ms(10U);
    (void)core_gpio_write_pin(CORE_GPIO_DISPLAY_RESET, CORE_GPIO_PIN_SET);
    display->delay_ms(120U);
    return ST7789_OK;
}

st7789_status_t st7789_init(st7789_t *display)
{
    uint8_t color_mode = 0x55U;
    uint8_t memory_access = 0x00U;

    if ((display == NULL) || (display->width == 0U) ||
        (display->height == 0U) || (display->delay_ms == NULL))
    {
        return ST7789_ERR_ARGUMENT;
    }

    display->initialized = 0U;
    if ((st7789_reset(display) != ST7789_OK) ||
        (st7789_command(display, ST7789_SWRESET) != ST7789_OK))
    {
        return ST7789_ERROR;
    }
    display->delay_ms(150U);

    if (st7789_command(display, ST7789_SLPOUT) != ST7789_OK)
    {
        return ST7789_ERROR;
    }
    display->delay_ms(120U);

    if ((st7789_command(display, ST7789_COLMOD) != ST7789_OK) ||
        (st7789_data(display, &color_mode, 1U) != ST7789_OK) ||
        (st7789_command(display, ST7789_MADCTL) != ST7789_OK) ||
        (st7789_data(display, &memory_access, 1U) != ST7789_OK) ||
        (st7789_command(display, ST7789_DISPON) != ST7789_OK))
    {
        return ST7789_ERROR;
    }

    display->delay_ms(20U);
    display->initialized = 1U;
    return ST7789_OK;
}

st7789_status_t st7789_set_window(st7789_t *display,
                                  uint16_t x,
                                  uint16_t y,
                                  uint16_t width,
                                  uint16_t height)
{
    uint16_t x_end;
    uint16_t y_end;
    uint8_t data[4];

    if ((display == NULL) || (display->initialized == 0U))
    {
        return ST7789_ERR_NOT_READY;
    }
    if ((width == 0U) || (height == 0U) || (x >= display->width) ||
        (y >= display->height) || (width > (display->width - x)) ||
        (height > (display->height - y)))
    {
        return ST7789_ERR_ARGUMENT;
    }

    x_end = (uint16_t)(x + width - 1U);
    y_end = (uint16_t)(y + height - 1U);
    data[0] = (uint8_t)(x >> 8);
    data[1] = (uint8_t)x;
    data[2] = (uint8_t)(x_end >> 8);
    data[3] = (uint8_t)x_end;
    if ((st7789_command(display, ST7789_CASET) != ST7789_OK) ||
        (st7789_data(display, data, sizeof(data)) != ST7789_OK))
    {
        return ST7789_ERROR;
    }

    data[0] = (uint8_t)(y >> 8);
    data[1] = (uint8_t)y;
    data[2] = (uint8_t)(y_end >> 8);
    data[3] = (uint8_t)y_end;
    if ((st7789_command(display, ST7789_RASET) != ST7789_OK) ||
        (st7789_data(display, data, sizeof(data)) != ST7789_OK) ||
        (st7789_command(display, ST7789_RAMWR) != ST7789_OK))
    {
        return ST7789_ERROR;
    }

    return ST7789_OK;
}

st7789_status_t st7789_fill(st7789_t *display, uint16_t color)
{
    uint8_t chunk[128];
    uint32_t remaining;
    uint16_t index;

    if ((display == NULL) || (display->initialized == 0U))
    {
        return ST7789_ERR_NOT_READY;
    }
    if (st7789_set_window(display, 0U, 0U, display->width, display->height) != ST7789_OK)
    {
        return ST7789_ERROR;
    }

    for (index = 0U; index < sizeof(chunk); index += 2U)
    {
        chunk[index] = (uint8_t)(color >> 8);
        chunk[index + 1U] = (uint8_t)color;
    }

    remaining = (uint32_t)display->width * display->height * 2U;
    while (remaining > 0U)
    {
        uint16_t size = (remaining > sizeof(chunk)) ? (uint16_t)sizeof(chunk) : (uint16_t)remaining;
        if (st7789_data(display, chunk, size) != ST7789_OK)
        {
            return ST7789_ERROR;
        }
        remaining -= size;
    }

    return ST7789_OK;
}

st7789_status_t st7789_draw_image(st7789_t *display,
                                  uint16_t x,
                                  uint16_t y,
                                  uint16_t width,
                                  uint16_t height,
                                  const uint16_t *pixels)
{
    uint32_t count;

    if (pixels == NULL)
    {
        return ST7789_ERR_ARGUMENT;
    }
    if (st7789_set_window(display, x, y, width, height) != ST7789_OK)
    {
        return ST7789_ERROR;
    }

    count = (uint32_t)width * height;
    while (count-- > 0U)
    {
        uint8_t data[2] = {(uint8_t)(*pixels >> 8), (uint8_t)*pixels};
        if (st7789_data(display, data, sizeof(data)) != ST7789_OK)
        {
            return ST7789_ERROR;
        }
        pixels++;
    }

    return ST7789_OK;
}

st7789_status_t st7789_sleep(st7789_t *display)
{
    if ((display == NULL) || (display->initialized == 0U))
    {
        return ST7789_ERR_NOT_READY;
    }
    return st7789_command(display, ST7789_SLPIN);
}

st7789_status_t st7789_wakeup(st7789_t *display)
{
    if ((display == NULL) || (display->initialized == 0U))
    {
        return ST7789_ERR_NOT_READY;
    }
    if (st7789_command(display, ST7789_SLPOUT) != ST7789_OK)
    {
        return ST7789_ERROR;
    }
    display->delay_ms(120U);
    return st7789_command(display, ST7789_DISPON);
}

