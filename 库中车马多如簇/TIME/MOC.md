# TIME

[← 模块总览](../MOC.md) | [← 主页](../../index.md) | [← FOC 工程代码](../FOC/MOC.md#工程代码索引)

---

## 代码索引

| 模块 | 代码 | 说明 |
| --- | --- | --- |
| 时基 | [time_port.c](./03_Firmware/04_platform/platform_mcu/time/src/time_port.c) / [time_port.h](./03_Firmware/04_platform/platform_mcu/time/inc/time_port.h) | 毫秒/微秒计时、HRTIM Master 与 TIM 启动、微秒延时 |

## 接口

- `core_time_us_init()`：初始化微秒时基。
- `core_time_get_us()` / `core_time_get_us_delta()`：读取并计算无符号回绕的微秒差。
- `core_hrtim_master_start()`：启动 HRTIM Master。
- `core_tim_start()`：启动指定 TIM。
- `core_time_get()` / `core_time_get_direct()`：毫秒和直接毫秒时基。
- `core_delay_us()`：微秒级阻塞延时。

## 依赖

- STM32 HAL，以及工程提供的 `stm32g4xx_hal.h`。
- DWT 计数器用于微秒计时。
