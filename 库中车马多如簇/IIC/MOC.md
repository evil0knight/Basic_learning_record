# IIC

[← 模块总览](../MOC.md) | [← 主页](../../index.md) | [← IIC知识](./知识/MOC.md)

---

I2C 代码分为通用分层版和 FOC 简化版，按工程实时性与功能需求选择。

| 场景 | 版本 | 实现方式 | 入口 |
| --- | --- | --- | --- |
| 通用工程 | 通用分层版 | Bus + 裸机/RTOS Port；请求队列、锁、超时和软硬件 I2C 分发 | [I2C Bus / Port](./03_Firmware/通用分层版/IIC_Port/MOC.md) |
| FOC 高实时 | FOC简化版 | `core_iic_*` 直接封装单路 HAL I2C，同步读写和 DMA，不建队列或逻辑端口 | [简化版代码](./03_Firmware/FOC简化版/04_platform/platform_mcu/i2c/inc/core_iic.h) |

## 文档入口

| 内容 | 文件 |
| --- | --- |
| 两个版本的统一索引 | [03_Firmware/MOC.md](./03_Firmware/MOC.md) |
| 通用版移植与使用 | [移植指南](./移植指南.md) |
| I2C 原理与协议 | [IIC知识](./知识/MOC.md) |
| FOC 工程代码总索引 | [FOC 工程代码](../FOC/MOC.md#工程代码索引) |
