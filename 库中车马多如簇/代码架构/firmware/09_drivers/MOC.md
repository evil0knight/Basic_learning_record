# 09_drivers

[返回工程根目录](../MOC.md) | [返回代码架构](../../MOC.md) | [返回主页](../../../../index.md)

## 关键文件

- `CMSIS/`：ARM 内核、芯片设备头文件和启动支持。
- `STM32G4xx_HAL_Driver/Inc/`：STM32G4 HAL 头文件。
- `STM32G4xx_HAL_Driver/Src/`：STM32G4 HAL 外设实现。
- `stm32g4xx_hal_conf.h`：HAL 模块选择和配置。

## 调用顺序

```text
CMSIS / STM32G4 HAL
    -> platform_mcu Port
    -> Platform Adapter
    -> BSP Driver
```
