# FOC 代码目录

[← MOC](../MOC.md) | [← 主页](../../index.md)
[← FOC 笔记 MOC](../../术中自有万钟粟/FOC/MOC.md)

---

## 代码索引

| 模块 | 代码 | 对应笔记 |
| --- | --- | --- |
| 位置环 | [position_loop.c](./src/position_loop.c) / [position_loop.h](./inc/position_loop.h) | [位置环](../../术中自有万钟粟/FOC/位置环.md) |
| 速度环 | [speed_loop.c](./src/speed_loop.c) / [speed_loop.h](./inc/speed_loop.h) | [速度环](../../术中自有万钟粟/FOC/速度环.md) |
| 电流环 | [current_loop.c](./src/current_loop.c) / [current_loop.h](./inc/current_loop.h) | [电流环](../../术中自有万钟粟/FOC/电流环.md) |
| Clarke变换 | [Clarke.c](./src/Clarke.c) / [foc_transform.h](./inc/foc_transform.h) | [Clarke变换](../../术中自有万钟粟/FOC/Clarke变换.md) |
| Park变换 | [Park.c](./src/Park.c) / [foc_transform.h](./inc/foc_transform.h) | [Park变换](../../术中自有万钟粟/FOC/Park变换.md) |
| 逆Park变换 | [Inverse_Park.c](./src/Inverse_Park.c) / [foc_transform.h](./inc/foc_transform.h) | [逆Park变换](../../术中自有万钟粟/FOC/逆Park变换.md) |
| SVPWM算法 | [svpwm.c](./src/svpwm.c) / [foc_transform.h](./inc/foc_transform.h) | [SVPWM算法](../../术中自有万钟粟/FOC/SVPWM算法.md) |
| PLL锁相环 | [pll.c](./src/pll.c) / [sin_lut.h](./inc/sin_lut.h) | [PLL锁相环](../../术中自有万钟粟/FOC/PLL锁相环.md) |
| 启动策略 | [startup_strategy.c](./src/startup_strategy.c) | [启动策略](../../术中自有万钟粟/FOC/启动策略.md) |
| MTPA磁场定向控制 | [mtpa.c](./src/mtpa.c) | [MTPA磁场定向控制](../../术中自有万钟粟/FOC/MTPA磁场定向控制.md) |
| 弱磁控制 | [field_weakening.c](./src/field_weakening.c) | [弱磁控制](../../术中自有万钟粟/FOC/弱磁控制.md) |
| sin/cos 查表 | [sin_lut.c](./src/sin_lut.c) / [sin_lut.h](./inc/sin_lut.h) | [Park变换](../../术中自有万钟粟/FOC/Park变换.md) / [逆Park变换](../../术中自有万钟粟/FOC/逆Park变换.md) |

## 工程代码索引

| 模块 | 工程代码 |
| --- | --- |
| FOC Service | [service_foc.h](./03_Firmware/FOCV2.5.4/02_service/service_foc/service_foc.h) / [初始化](./03_Firmware/FOCV2.5.4/02_service/service_foc/service_foc_init.c) / [双电机](./03_Firmware/FOCV2.5.4/02_service/service_foc/service_foc_motor1.c) / [零负载标定](./03_Firmware/FOCV2.5.4/02_service/service_foc/service_foc_zero_load/service_foc_zero_load.c) |
| 电流 Wrapper | [bsp_wrapper_current.c](./03_Firmware/FOCV2.5.4/04_platform/platform_bsp/current/wrapper/bsp_wrapper_current.c) / [bsp_wrapper_current.h](./03_Firmware/FOCV2.5.4/04_platform/platform_bsp/current/wrapper/bsp_wrapper_current.h) |
| 编码器 Wrapper | [bsp_wrapper_encoder.c](../编码器/03_Firmware/FOC实时版/04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.c) / [bsp_wrapper_encoder.h](../编码器/03_Firmware/FOC实时版/04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h) |
| ADC | [adc_port.c](../ADC/03_Firmware/FOC简化版/04_platform/platform_mcu/adc/src/adc_port.c) / [adc_port.h](../ADC/03_Firmware/FOC简化版/04_platform/platform_mcu/adc/inc/adc_port.h) |
| I2C | [core_iic.c](../IIC/03_Firmware/FOC简化版/04_platform/platform_mcu/i2c/src/core_iic.c) / [core_iic.h](../IIC/03_Firmware/FOC简化版/04_platform/platform_mcu/i2c/inc/core_iic.h) |
| TIME | [time_port.c](../TIME/03_Firmware/04_platform/platform_mcu/time/src/time_port.c) / [time_port.h](../TIME/03_Firmware/04_platform/platform_mcu/time/inc/time_port.h) |
| Critical | [critical_port.c](../代码架构/frimware_bare/04_platform/platform_mcu/critical/src/critical_port.c) / [critical_port.h](../代码架构/frimware_bare/04_platform/platform_mcu/critical/inc/critical_port.h) |
