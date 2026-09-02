# GPIO Port

[← platform_mcu](../MOC.md) | [← 代码架构库](../../../../MOC.md) | [← 主页](../../../../../../index.md)

---

| 文件 | 作用 |
| --- | --- |
| [gpio_port.h](./inc/gpio_port.h) | 逻辑GPIO、注册配置、逻辑接口和底层直接接口 |
| [gpio_port.c](./src/gpio_port.c) | 静态注册表及HAL GPIO实现 |

## 接入

1. 在CubeMX配置GPIO并设置User Label，物理宏生成到`main.h`。
2. 在项目初始化代码创建配置并注册逻辑位置：

```c
static const st_core_gpio_config_t display_dc = {
    .port = LCD_DC_GPIO_Port,
    .pin = LCD_DC_Pin,
    .default_state = CORE_GPIO_PIN_RESET,
    .initialize_output = true
};

core_gpio_register(CORE_GPIO_DISPLAY_DC, &display_dc);
core_gpio_init();
```

3. Driver只调用`core_gpio_write_pin(CORE_GPIO_DISPLAY_DC, ...)`，不使用物理端口。

## 接口边界

| 接口 | 使用位置 |
| --- | --- |
| `core_gpio_write/read/toggle_pin()` | 正式设备功能 |
| `core_gpio_*_pin_direct()` | Platform底层动态引脚、SPI Port和硬件调试 |
| `HAL_GPIO_*()` | 仅GPIO Port实现 |

LED、按键、DC、RST等长期使用的信号应注册逻辑ID。`direct`不是更快的业务接口，而是动态底层配置的出口。
