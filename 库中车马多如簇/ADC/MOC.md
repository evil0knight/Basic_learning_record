# ADC

[← 模块总览](../MOC.md) | [← 主页](../../index.md)
[← 返回 电流采样与处理](../../术中自有万钟粟/FOC/电流采样与处理.md)

---

ADC 代码分为通用双缓冲版和 FOC 简化版。

| 场景 | 版本 | 实现方式 | 入口 |
| --- | --- | --- | --- |
| 通用工程、RTOS | 通用双缓冲版 | App 中断入口 + Platform 注册表 + Service 双缓冲双任务 | [通用双缓冲版](./03_Firmware/通用双缓冲版/MOC.md) |
| FOC 高实时 | FOC简化版 | 增加 Injected 组、校准和注入回调；由 FOC 中断直接消费 | [简化版代码](./03_Firmware/FOC简化版/04_platform/platform_mcu/adc/inc/adc_port.h) |

## 文档入口

| 内容 | 文件 |
| --- | --- |
| 两个版本的统一索引 | [03_Firmware/MOC.md](./03_Firmware/MOC.md) |
| 通用版详细说明 | [通用双缓冲版 MOC](./03_Firmware/通用双缓冲版/MOC.md) |
| 电流采样笔记 | [电流采样与处理](../../术中自有万钟粟/FOC/电流采样与处理.md) |
| FOC 工程代码总索引 | [FOC 工程代码](../FOC/MOC.md#工程代码索引) |
