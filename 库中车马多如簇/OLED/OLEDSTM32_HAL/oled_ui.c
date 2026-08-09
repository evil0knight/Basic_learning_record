#include "oled_ui.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#define OLED_UI_SCREEN_WIDTH       SSD1306_WIDTH
#define OLED_UI_SCREEN_HEIGHT      SSD1306_HEIGHT
#define OLED_UI_TREND_LEFT         4U
#define OLED_UI_TREND_TOP          14U
#define OLED_UI_TREND_WIDTH        120U
#define OLED_UI_TREND_HEIGHT       40U
#define OLED_UI_MENU_MAX_ITEMS     5U

static void OLED_UI_DrawSun(uint8_t x, uint8_t y);
static void OLED_UI_DrawBattery(uint8_t x, uint8_t y);
static void OLED_UI_DrawGear(uint8_t x, uint8_t y);
static uint8_t OLED_UI_ClampSample(uint8_t sample);

void OLED_UI_Init(void)
{
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}

void OLED_UI_Clear(void)
{
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}

void OLED_UI_ShowTrend(const char *title,
                       const char *value_text,
                       const uint8_t *samples,
                       uint8_t sample_count)
{
    uint8_t i;
    uint8_t prev_x;
    uint8_t prev_y;

    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString((char *)(title ? title : "Trend"), Font_7x10, White);

    if (value_text != 0) {
        ssd1306_SetCursor(88, 0);
        ssd1306_WriteString((char *)value_text, Font_7x10, White);
    }

    ssd1306_Line(OLED_UI_TREND_LEFT,
                 (uint8_t)(OLED_UI_TREND_TOP + OLED_UI_TREND_HEIGHT),
                 (uint8_t)(OLED_UI_TREND_LEFT + OLED_UI_TREND_WIDTH),
                 (uint8_t)(OLED_UI_TREND_TOP + OLED_UI_TREND_HEIGHT),
                 White);
    ssd1306_Line(OLED_UI_TREND_LEFT,
                 OLED_UI_TREND_TOP,
                 OLED_UI_TREND_LEFT,
                 (uint8_t)(OLED_UI_TREND_TOP + OLED_UI_TREND_HEIGHT),
                 White);

    if ((samples != 0) && (sample_count >= 2U)) {
        prev_x = (uint8_t)(OLED_UI_TREND_LEFT + 1U);
        prev_y = (uint8_t)(OLED_UI_TREND_TOP + OLED_UI_TREND_HEIGHT -
                           ((uint16_t)OLED_UI_ClampSample(samples[0]) * (OLED_UI_TREND_HEIGHT - 2U)) / 100U);

        for (i = 1U; i < sample_count; i++) {
            uint8_t x = (uint8_t)(OLED_UI_TREND_LEFT + 1U +
                        ((uint16_t)(OLED_UI_TREND_WIDTH - 2U) * i) / (sample_count - 1U));
            uint8_t y = (uint8_t)(OLED_UI_TREND_TOP + OLED_UI_TREND_HEIGHT -
                        ((uint16_t)OLED_UI_ClampSample(samples[i]) * (OLED_UI_TREND_HEIGHT - 2U)) / 100U);

            ssd1306_Line(prev_x, prev_y, x, y, White);
            prev_x = x;
            prev_y = y;
        }
    }

    ssd1306_UpdateScreen();
}

void OLED_UI_ShowMenu(const char *title,
                      const char *const *items,
                      uint8_t item_count,
                      uint8_t selected_index)
{
    uint8_t i;
    uint8_t y;

    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString((char *)(title ? title : "Menu"), Font_7x10, White);

    if (item_count > OLED_UI_MENU_MAX_ITEMS) {
        item_count = OLED_UI_MENU_MAX_ITEMS;
    }

    for (i = 0U; i < item_count; i++) {
        y = (uint8_t)(16U + i * 12U);

        if (i == selected_index) {
            ssd1306_DrawRectangle(0, (uint8_t)(y - 2U), (uint8_t)(OLED_UI_SCREEN_WIDTH - 1U), (uint8_t)(y + 9U), White);
            ssd1306_FillRectangle(3, (uint8_t)(y + 2U), 7, (uint8_t)(y + 6U), White);
        }

        ssd1306_SetCursor(14, (uint8_t)(y - 1U));
        ssd1306_WriteString((char *)((items != 0 && items[i] != 0) ? items[i] : ""), Font_7x10, White);
    }

    ssd1306_UpdateScreen();
}

void OLED_UI_ShowIcons(void)
{
    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("Icons", Font_7x10, White);

    OLED_UI_DrawIcon(OLED_UI_ICON_SUN, 20, 24);
    OLED_UI_DrawIcon(OLED_UI_ICON_BATTERY, 60, 24);
    OLED_UI_DrawIcon(OLED_UI_ICON_GEAR, 102, 24);

    ssd1306_SetCursor(6, 44);
    ssd1306_WriteString("Sun", Font_6x8, White);
    ssd1306_SetCursor(48, 44);
    ssd1306_WriteString("Batt", Font_6x8, White);
    ssd1306_SetCursor(93, 44);
    ssd1306_WriteString("Gear", Font_6x8, White);

    ssd1306_UpdateScreen();
}

void OLED_UI_DrawIcon(OLED_UI_Icon_t icon, uint8_t x, uint8_t y)
{
    switch (icon) {
    case OLED_UI_ICON_SUN:
        OLED_UI_DrawSun(x, y);
        break;
    case OLED_UI_ICON_BATTERY:
        OLED_UI_DrawBattery(x, y);
        break;
    case OLED_UI_ICON_GEAR:
        OLED_UI_DrawGear(x, y);
        break;
    default:
        break;
    }
}

static void OLED_UI_DrawSun(uint8_t x, uint8_t y)
{
    ssd1306_DrawCircle(x, y, 10, White);
    ssd1306_Line(x, (uint8_t)(y - 14), x, (uint8_t)(y - 10), White);
    ssd1306_Line(x, (uint8_t)(y + 10), x, (uint8_t)(y + 14), White);
    ssd1306_Line((uint8_t)(x - 14), y, (uint8_t)(x - 10), y, White);
    ssd1306_Line((uint8_t)(x + 10), y, (uint8_t)(x + 14), y, White);
    ssd1306_Line((uint8_t)(x - 9), (uint8_t)(y - 9), (uint8_t)(x - 6), (uint8_t)(y - 6), White);
    ssd1306_Line((uint8_t)(x + 6), (uint8_t)(y + 6), (uint8_t)(x + 9), (uint8_t)(y + 9), White);
    ssd1306_Line((uint8_t)(x - 9), (uint8_t)(y + 9), (uint8_t)(x - 6), (uint8_t)(y + 6), White);
    ssd1306_Line((uint8_t)(x + 6), (uint8_t)(y - 6), (uint8_t)(x + 9), (uint8_t)(y - 9), White);
}

static void OLED_UI_DrawBattery(uint8_t x, uint8_t y)
{
    ssd1306_DrawRectangle((uint8_t)(x - 15), (uint8_t)(y - 10), (uint8_t)(x + 15), (uint8_t)(y + 10), White);
    ssd1306_FillRectangle((uint8_t)(x + 17), (uint8_t)(y - 4), (uint8_t)(x + 20), (uint8_t)(y + 4), White);
    ssd1306_FillRectangle((uint8_t)(x - 12), (uint8_t)(y - 7), x, (uint8_t)(y + 7), White);
}

static void OLED_UI_DrawGear(uint8_t x, uint8_t y)
{
    ssd1306_DrawCircle(x, y, 10, White);
    ssd1306_DrawCircle(x, y, 4, White);
    ssd1306_Line(x, (uint8_t)(y - 14), x, (uint8_t)(y - 10), White);
    ssd1306_Line(x, (uint8_t)(y + 10), x, (uint8_t)(y + 14), White);
    ssd1306_Line((uint8_t)(x - 14), y, (uint8_t)(x - 10), y, White);
    ssd1306_Line((uint8_t)(x + 10), y, (uint8_t)(x + 14), y, White);
    ssd1306_Line((uint8_t)(x - 7), (uint8_t)(y - 7), (uint8_t)(x - 11), (uint8_t)(y - 11), White);
    ssd1306_Line((uint8_t)(x + 7), (uint8_t)(y + 7), (uint8_t)(x + 11), (uint8_t)(y + 11), White);
    ssd1306_Line((uint8_t)(x - 7), (uint8_t)(y + 7), (uint8_t)(x - 11), (uint8_t)(y + 11), White);
    ssd1306_Line((uint8_t)(x + 7), (uint8_t)(y - 7), (uint8_t)(x + 11), (uint8_t)(y - 11), White);
}

static uint8_t OLED_UI_ClampSample(uint8_t sample)
{
    if (sample > 100U) {
        return 100U;
    }
    return sample;
}
