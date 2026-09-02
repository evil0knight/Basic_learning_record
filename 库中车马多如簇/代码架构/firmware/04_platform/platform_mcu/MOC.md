# platform_mcu

[← 代码架构库](../../../MOC.md) | [← 嵌入式工程架构](../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../MOC.md) | [← 主页](../../../../../index.md)

---

按 GPIO、I2C、SPI、USART、ADC 等 MCU 外设建立统一接口，对上层隐藏 HAL 句柄和芯片差异。

每个外设使用 inc 存放公开头文件，使用 src 存放具体 MCU 实现。

| 文件                                     | 作用                             |
| ---------------------------------------- | -------------------------------- |
| [GPIO Port](./gpio/MOC.md)                  | GPIO注册、逻辑接口和底层直接接口 |
| [I2C Port](../../../../IIC/IIC_Port/MOC.md) | I2C静态总线、硬件/软件接口和锁管理 |
| `platform_mcu/usart/inc/usart_port.h`  | USART统一接口                    |
| `platform_mcu/usart/src/usart_port.c`  | USART平台实现                    |
| `platform_mcu/adc/inc/adc_port.h`      | ADC统一接口                      |
| `platform_mcu/adc/src/adc_port.c`      | ADC平台实现                      |
| [SPI Port](../../../../SPI/SPI_Port/MOC.md) | SPI注册、逻辑接口和底层直接接口  |

## platform_mcu配置:

1. 按外设新建目录，例如 `platform_mcu/gpio/inc/`和 `platform_mcu/gpio/src/`。
2. 在 `inc/gpio_port.h` 声明统一接口：

```c
void gpio_port_init(void);
void gpio_port_write(uint32_t pin, bool level);
bool gpio_port_read(uint32_t pin);
```

3. 通用Port源码保存空注册表，不写具体板卡的端口、引脚和HAL句柄。
4. 在项目初始化代码中把 `main.h`的物理资源注册到统一逻辑位置。
5. Driver调用 `*_port.h`，不调用HAL；I2C、USART、ADC按相同结构建立。
