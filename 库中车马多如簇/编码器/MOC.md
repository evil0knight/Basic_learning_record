# 编码器

[← MOC](../MOC.md) | [← 主页](../../index.md)

---

AS5600 的两套工程实现按实时性要求分开索引。两版不能同时加入同一工程。

| 场景 | 版本 | 实现方式 | 入口 |
| --- | --- | --- | --- |
| 实时性要求不高 | 通用分层版 | Service → Adapter → Wrapper → Handler → Driver；总线、队列和超时分层完整 | [通用分层版](./03_Firmware/通用分层版/MOC.md) |
| 实时性要求高 | FOC实时版 | 删除 BSP 的 Adapter、Handler、Driver；Wrapper 直接调用简化 I2C 和微秒时基，5 kHz DMA 采样 | [FOC实时版](./03_Firmware/MOC.md#foc实时版) |

两版的公共约束：AS5600 地址为 `0x36`，使用 8 位寄存器地址，角度计数为 12 位。多颗同地址器件需要独立总线或通道切换。
