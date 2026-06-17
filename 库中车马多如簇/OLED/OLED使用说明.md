# SSD1306 OLED 使用说明

这份说明是给“别的工程移植使用”准备的，只讲怎么使用这组 OLED 文件。

## 1. 需要拷贝哪些文件

把下面文件拷贝到你的工程里：

- `ssd1306.h`
- `ssd1306_conf.h`
- `ssd1306_fonts.h`
- `ssd1306.c`
- `ssd1306_fonts.c`
- `oled_ui.h`
- `oled_ui.c`

如果你还想参考官方演示，可以额外拷贝：

- `ssd1306_tests.h`
- `ssd1306_tests.c`

## 2. 使用前要准备什么

这套驱动依赖 `STM32 HAL`。

你的工程里要满足：

- 已经能正常使用 HAL
- 已经初始化好 I2C 或 SPI
- `ssd1306_conf.h` 里的配置和你的硬件一致

当前这套代码默认是 `I2C` 模式。

## 3. 先改 `ssd1306_conf.h`

### 3.1 选择单片机系列

按你的芯片修改，例如：

```c
#define STM32F1
```

如果你是别的系列，就改成对应宏，比如：

```c
// #define STM32F4
// #define STM32L4
```

### 3.2 选择通信方式

如果你用 I2C：

```c
#define SSD1306_USE_I2C
```

如果你用 SPI：

```c
// #define SSD1306_USE_SPI
```

二选一。

### 3.3 配置 I2C

如果你用 I2C，要改这两个：

```c
#define SSD1306_I2C_PORT hi2c1
#define SSD1306_I2C_ADDR (0x3C << 1)
```

说明：

- `SSD1306_I2C_PORT` 要和你工程里的 I2C 句柄名字一致
- `SSD1306_I2C_ADDR` 一般是 `(0x3C << 1)` 或 `(0x3D << 1)`

例如你工程里是 `hi2c2`，就改成：

```c
#define SSD1306_I2C_PORT hi2c2
```

### 3.4 配置 SPI

如果你用 SPI，就启用这些宏并改成你的引脚：

```c
#define SSD1306_SPI_PORT   hspi1
#define SSD1306_CS_Port    OLED_CS_GPIO_Port
#define SSD1306_CS_Pin     OLED_CS_Pin
#define SSD1306_DC_Port    OLED_DC_GPIO_Port
#define SSD1306_DC_Pin     OLED_DC_Pin
#define SSD1306_Reset_Port OLED_Res_GPIO_Port
#define SSD1306_Reset_Pin  OLED_Res_Pin
```

### 3.5 配置屏幕尺寸

默认是 `128x64`。

如果你的屏不是这个尺寸，可以改：

```c
#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64
```

### 3.6 选择字体

按需启用：

```c
#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18
```

不用的字体可以注释掉，减少 Flash 占用。

## 4. 工程里要包含哪些头文件

使用 OLED 时一般包含：

```c
#include "ssd1306.h"
#include "ssd1306_fonts.h"
```

如果你想直接使用已经封装好的页面接口，再包含：

```c
#include "oled_ui.h"
```

## 5. 初始化顺序

正确顺序是：

1. `HAL_Init()`
2. 系统时钟初始化
3. I2C 或 SPI 初始化
4. `ssd1306_Init()`

例如：

```c
HAL_Init();
SystemClock_Config();
MX_I2C1_Init();

ssd1306_Init();
```

## 6. 最小显示例程

```c
#include "ssd1306.h"
#include "ssd1306_fonts.h"

void OLED_Demo(void)
{
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("Hello OLED", Font_7x10, White);
    ssd1306_UpdateScreen();
}
```

注意：

- `ssd1306_Fill()` 只是清缓冲区
- `ssd1306_WriteString()` 只是写缓冲区
- `ssd1306_UpdateScreen()` 才是真正刷新到屏幕

## 7. 如果直接使用封装好的 UI 接口

`oled_ui.c/.h` 已经封装了 3 类页面：

- 趋势图
- 菜单
- 图标页

### 7.1 初始化

```c
OLED_UI_Init();
```

### 7.2 清屏

```c
OLED_UI_Clear();
```

### 7.3 显示趋势图

```c
uint8_t samples[] = {20, 35, 28, 52, 67, 60, 75, 80};

OLED_UI_ShowTrend("Trend", "80%", samples, 8);
```

参数说明：

- 第 1 个参数：标题
- 第 2 个参数：右上角数值文本
- 第 3 个参数：趋势数据，范围建议 `0~100`
- 第 4 个参数：数据个数

### 7.4 显示菜单

```c
const char *items[] = {"CAN View", "Settings", "About"};

OLED_UI_ShowMenu("Menu", items, 3, 1);
```

参数说明：

- 第 1 个参数：标题
- 第 2 个参数：菜单项字符串数组
- 第 3 个参数：菜单项个数
- 第 4 个参数：当前选中项下标，从 `0` 开始

### 7.5 显示图标页

```c
OLED_UI_ShowIcons();
```

### 7.6 单独画图标

```c
OLED_UI_DrawIcon(OLED_UI_ICON_SUN, 20, 24);
OLED_UI_DrawIcon(OLED_UI_ICON_BATTERY, 60, 24);
OLED_UI_DrawIcon(OLED_UI_ICON_GEAR, 102, 24);
ssd1306_UpdateScreen();
```

注意：

- `OLED_UI_ShowTrend()`、`OLED_UI_ShowMenu()`、`OLED_UI_ShowIcons()` 内部已经会自动刷新屏幕
- `OLED_UI_DrawIcon()` 只负责绘制，不会自动刷新，画完要自己调用 `ssd1306_UpdateScreen()`

## 8. 常用底层函数怎么用

### 8.1 初始化

```c
ssd1306_Init();
```

### 8.2 清屏

```c
ssd1306_Fill(Black);
ssd1306_UpdateScreen();
```

### 8.3 显示字符串

```c
ssd1306_SetCursor(0, 0);
ssd1306_WriteString("CAN TEST", Font_11x18, White);
ssd1306_UpdateScreen();
```

### 8.4 显示单个字符

```c
ssd1306_SetCursor(0, 0);
ssd1306_WriteChar('A', Font_16x24, White);
ssd1306_UpdateScreen();
```

### 8.5 画点

```c
ssd1306_DrawPixel(10, 10, White);
ssd1306_UpdateScreen();
```

### 8.6 画线

```c
ssd1306_Line(0, 0, 127, 63, White);
ssd1306_UpdateScreen();
```

### 8.7 画矩形

```c
ssd1306_DrawRectangle(10, 10, 60, 30, White);
ssd1306_UpdateScreen();
```

### 8.8 画填充矩形

```c
ssd1306_FillRectangle(10, 10, 60, 30, White);
ssd1306_UpdateScreen();
```

### 8.9 画圆

```c
ssd1306_DrawCircle(64, 32, 10, White);
ssd1306_UpdateScreen();
```

### 8.10 画填充圆

```c
ssd1306_FillCircle(64, 32, 10, White);
ssd1306_UpdateScreen();
```

### 8.11 显示位图

```c
ssd1306_DrawBitmap(0, 0, bmp, 128, 64, White);
ssd1306_UpdateScreen();
```

其中 `bmp` 是你提前准备好的位图数组。

## 9. `TREND`、`MENU`、`ICONS` 这些功能现在是什么形式

现在这三个功能已经封装成独立接口：

- `OLED_UI_ShowTrend()`
- `OLED_UI_ShowMenu()`
- `OLED_UI_ShowIcons()`
- `OLED_UI_DrawIcon()`

它们属于应用层封装，不属于 `ssd1306.c/.h` 的底层驱动接口。

底层驱动仍然只负责基础绘图。

上层 `oled_ui.c/.h` 则负责把这些基础函数组合成可直接复用的页面。
## 10. 如果你不想用 `oled_ui.c/.h`

### 10.1 `TREND`

趋势图通常这样做：

- 先画坐标轴：`ssd1306_Line()`
- 再根据数据点画折线：`ssd1306_Line()`
- 再写标题和数值：`ssd1306_WriteString()`

例如：

```c
ssd1306_Fill(Black);
ssd1306_WriteString("Trend", Font_7x10, White);
ssd1306_Line(4, 54, 124, 54, White);
ssd1306_Line(4, 14, 4, 54, White);
/* 然后继续画折线 */
ssd1306_UpdateScreen();
```

### 10.2 `MENU`

菜单通常这样做：

- 写菜单标题：`ssd1306_WriteString()`
- 写每一项文字：`ssd1306_WriteString()`
- 用矩形框选当前项：`ssd1306_DrawRectangle()`
- 用小块或箭头做选中标记：`ssd1306_FillRectangle()` 或位图

例如：

```c
ssd1306_Fill(Black);
ssd1306_WriteString("Menu", Font_7x10, White);
ssd1306_DrawRectangle(0, 14, 127, 28, White);
ssd1306_WriteString("CAN View", Font_7x10, White);
ssd1306_UpdateScreen();
```

### 10.3 `ICONS`

图标有两种常见做法：

第一种，直接用基础图形拼：

- 圆：`ssd1306_DrawCircle()`
- 线：`ssd1306_Line()`
- 矩形：`ssd1306_DrawRectangle()`

第二种，直接显示位图：

- `ssd1306_DrawBitmap()`

如果图标固定不变，推荐用位图，简单直接。

## 11. 最后记住这三点

- 先初始化 I2C 或 SPI，再 `ssd1306_Init()`
- 所有绘图先写缓冲区
- 每次显示完成后必须 `ssd1306_UpdateScreen()`
