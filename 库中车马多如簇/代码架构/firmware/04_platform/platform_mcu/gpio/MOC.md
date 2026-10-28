# GPIO Port

[← platform_mcu](../MOC.md) | [← firmware](../../../MOC.md) | [← 主页](../../../../../../index.md)

---

| 文件                                        | 作用                                           |
| ------------------------------------------- | ---------------------------------------------- |
| [gpio_port.h](./inc/gpio_port.h)               | 逻辑 GPIO ID、配置类型、逻辑接口和底层直接接口 |
| [gpio_port_config.h](./inc/gpio_port_config.h) | 板级引脚映射和默认电平静态配置表               |
| [gpio_port.c](./src/gpio_port.c)               | 读取配置表，实现初始化和 HAL GPIO 操作         |

这里就是在platform直接把hal层的做一层薄封装,然后放在同一初始化的地方用这里集成的信息,去调用薄封装过后的hal的初始化函数

## 接入

1. 在CubeMX配置GPIO并设置User Label，物理宏生成到 `main.h`。
2. 在 `gpio_port_config.h` 的 `g_gpio_configs[]` 中填写板级引脚映射。
   以下示例要求目标工程已定义 `LCD_DC_GPIO_Port` 和 `LCD_DC_Pin`,没有设置User Label就直接填写Port和pin

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

3. `gpio_port.h`里写一下en_core_gpio_pin_t,这个是解耦之后对引脚的叫法
4. 初始化调用 `core_gpio_init()`,这个应该是在[user_init.c](..\..\..\01_app\app_init\user_init.c)的 `userTaskInitFunction`去初始化
5. 在设备 Adapter 的平台函数中调用
   `core_gpio_write_pin(CORE_GPIO_DISPLAY_DC, ...)`，Driver 通过 `pf_*`接口调用 Adapter
