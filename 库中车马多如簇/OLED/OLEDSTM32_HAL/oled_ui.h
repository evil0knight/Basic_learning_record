#ifndef __OLED_UI_H__
#define __OLED_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    OLED_UI_ICON_SUN = 0,
    OLED_UI_ICON_BATTERY,
    OLED_UI_ICON_GEAR
} OLED_UI_Icon_t;

void OLED_UI_Init(void);
void OLED_UI_Clear(void);

void OLED_UI_ShowTrend(const char *title,
                       const char *value_text,
                       const uint8_t *samples,
                       uint8_t sample_count);

void OLED_UI_ShowMenu(const char *title,
                      const char *const *items,
                      uint8_t item_count,
                      uint8_t selected_index);

void OLED_UI_ShowIcons(void);
void OLED_UI_DrawIcon(OLED_UI_Icon_t icon, uint8_t x, uint8_t y);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_UI_H__ */
