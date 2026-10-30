# GPIO Port

[← platform_mcu](../MOC.md) | [← frimware_bare](../../../MOC.md) | [← 主页](../../../../../../index.md)

---

| 文件                                          | 作用                                           |
| --------------------------------------------- | ---------------------------------------------- |
| [gpio_port.h](./inc/gpio_port.h)               | 逻辑 GPIO ID、配置类型、逻辑接口和底层直接接口 |
| [gpio_port_config.h](./inc/gpio_port_config.h) | 板级引脚映射和默认电平静态配置表               |
| [gpio_port.c](./src/gpio_port.c)               | 读取配置表，实现初始化和 HAL GPIO 操作         |

## 接入

1. 在CubeMX配置GPIO并设置User Label，物理宏生成到`main.h`。
2. 在 `gpio_port_config.h` 的 `g_gpio_configs[]` 中填写板级引脚映射。
   以下示例要求目标工程已定义 `LCD_DC_GPIO_Port` 和 `LCD_DC_Pin`：

```c
static const st_core_gpio_config_t g_gpio_configs[CORE_GPIO_PIN_MAX] = {
    [CORE_GPIO_DISPLAY_DC] = {
        .port = LCD_DC_GPIO_Port,
        .pin = LCD_DC_Pin,
        .default_state = CORE_GPIO_PIN_RESET,
        .initialize_output = true
    }
};
```

3. 在 CubeMX 生成的 GPIO 初始化完成后调用 `core_gpio_init()`，应用已配置
   输出引脚的默认电平。该接口不配置时钟、输入输出模式、上下拉或中断。
4. 在设备 Adapter 的平台函数中调用
   `core_gpio_write_pin(CORE_GPIO_DISPLAY_DC, ...)`，Driver 通过 `pf_*`
   接口调用 Adapter。

配置表只由 `gpio_port.c` 包含，公共 `gpio_port.h` 不包含配置头。
未使用的表项保持零初始化，写入或翻转时返回
`CORE_GPIO_ERR_NOT_CONFIGURED`；读取时返回 `CORE_GPIO_PIN_RESET`。
引脚映射在编译时确定，无需调用注册函数。

读取接口直接通过返回值取得电平，无需传入输出指针：

```c
en_core_gpio_pin_state_t state = core_gpio_read_pin(CORE_GPIO_KEY1);
GPIO_PinState raw_state = core_gpio_read_pin_direct(LCD_DC_GPIO_Port,
                                                   LCD_DC_Pin);
```

逻辑读取返回 `CORE_GPIO_PIN_RESET` 或 `CORE_GPIO_PIN_SET`；
物理读取返回 `GPIO_PIN_RESET` 或 `GPIO_PIN_SET`。
逻辑标识无效、引脚未配置、物理端口为空或引脚掩码为零时，
对应读取接口返回低电平，无法通过返回值区分这些情况与正常低电平。
