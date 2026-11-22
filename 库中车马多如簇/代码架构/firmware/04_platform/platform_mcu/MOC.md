# platform_mcu

[返回 firmware](../../MOC.md) | [返回代码架构](../../../../MOC.md) | [返回主页](../../../../../index.md)

## 关键文件

- [gpio/MOC.md](./gpio/MOC.md)：说明 GPIO Port 的统一接口和 MCU 实现。
- [UART/MOC.md](../../../../UART/MOC.md)：说明 UART 的统一接口和平台实现。
- [ADC/MOC.md](../../../../ADC/MOC.md)：说明 ADC 的统一接口和平台实现。
- [IIC/IIC_Port/MOC.md](../../../../IIC/03_Firmware/通用分层版/IIC_Port/MOC.md)：说明 IIC Port 接口和底层实现。
- [SPI/SPI_Port/MOC.md](../../../../SPI/SPI_Port/MOC.md)：说明 SPI Port 接口和底层实现。
- [TIME/MOC.md](../../../../TIME/MOC.md)：说明毫秒/微秒时基及 HRTIM/TIM 启动。

## 调用顺序

```text
Platform Adapter
    -> platform_mcu Port 接口
    -> MCU 外设实现
    -> HAL
```
