# I2C 工程代码

[← IIC 总览](../MOC.md) | [← FOC 工程代码](../../FOC/MOC.md#工程代码索引) | [← 主页](../../../index.md)

---

## 通用分层版

包含 Bus、裸机 Port、RTOS Port 和 HAL 中断入口，适合多总线、软硬件 I2C 混用及 RTOS 场景。

| 模块 | 代码 | 说明 |
| --- | --- | --- |
| Bus | [i2c_bus.c](./通用分层版/IIC_Port/bus/src/i2c_bus.c) / [i2c_bus.h](./通用分层版/IIC_Port/bus/inc/i2c_bus.h) | 协议、HAL 分发、DMA、busy 和中断分发 |
| 软件 I2C | [soft_i2c.c](./通用分层版/IIC_Port/bus/src/soft_i2c.c) / [soft_i2c.h](./通用分层版/IIC_Port/bus/inc/soft_i2c.h) | GPIO 位翻转时序 |
| 裸机 Port | [i2c_port_bare.c](./通用分层版/IIC_Port/port/bare/i2c_port_bare.c) / [i2c_port_bare.h](./通用分层版/IIC_Port/port/bare/i2c_port_bare.h) | 请求队列、临界区、超时 |
| RTOS Port | [i2c_port_os.c](./通用分层版/IIC_Port/port/os/i2c_port_os.c) / [i2c_port_os.h](./通用分层版/IIC_Port/port/os/i2c_port_os.h) | 每总线 mutex/sema |
| HAL 中断入口 | [user_isr_handlers.c](./通用分层版/01_app/app_isr_handlers/user_isr_handlers.c) | 转发 HAL I2C 完成和错误回调 |

完整接口和移植说明见 [I2C Bus / Port](./通用分层版/IIC_Port/MOC.md) 与 [移植指南](../移植指南.md)。

## FOC简化版

只保留单路硬件 I2C 的直接 HAL 封装，供编码器等高频器件使用。

| 文件 | 作用 |
| --- | --- |
| [core_iic.c](./FOC简化版/04_platform/platform_mcu/i2c/src/core_iic.c) | 同步寄存器读写、双字节掩码更新、DMA 读启动和中止 |
| [core_iic.h](./FOC简化版/04_platform/platform_mcu/i2c/inc/core_iic.h) | `en_core_iic_status_t` 和对外接口 |

- 不建立逻辑 Port、请求队列、mutex 或 semaphore。
- 直接访问工程中的 `hi2c1`，硬件初始化由 CubeMX 的 `MX_I2C1_Init()` 完成。
- DMA 完成和错误仍由工程侧 `HAL_I2C_*Callback` 转发。
- 超时和寄存器更新延时使用固定宏，适合控制路径中的少量寄存器访问。

## 两版差异

| 对比项 | 通用分层版 | FOC简化版 |
| --- | --- | --- |
| 总线数量 | 多总线槽位 | 单路硬件 I2C |
| 软件 I2C | 支持 | 不支持 |
| 并发模型 | 裸机队列或 RTOS 锁 | 调用方保证串行访问 |
| 接口粒度 | 请求对象、同步/异步 | 直接同步/DMA |
| 适用 | 通用外设、RTOS、多器件 | 高频编码器、FOC 中断 |
