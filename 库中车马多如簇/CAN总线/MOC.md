# CAN总线

[← 模块总览](../MOC.md) | [← 主页](../../index.md) | [← CAN知识](./知识/MOC.md)

[← OTA](../../术中自有万钟粟/OTA/MOC.md)

---

CAN 代码分为通用分层版和 FOC 简化版，按工程实时性与功能需求选择。

| 场景 | 版本 | 实现方式 | 入口 |
| --- | --- | --- | --- |
| 通用工程 | 通用分层版 | 经典 CAN / CAN FD + 实例表 + 回调注册 | [通用分层版](./03_Firmware/通用分层版/04_platform/platform_mcu/can/inc/can_port.h) |
| FOC 高实时 | FOC简化版 | 单路经典 CAN / CAN FD，同步/异步发送 + FIFO 轮询 | [简化版代码](./03_Firmware/FOC简化版/04_platform/platform_mcu/can/inc/core_can.h) |

## 文档入口

| 内容 | 文件 |
| --- | --- |
| 两个版本的统一索引 | [03_Firmware/MOC.md](./03_Firmware/MOC.md) |
| 通用版移植与使用 | [移植指南](./移植指南.md) |
| CAN 原理与协议 | [CAN知识](./知识/MOC.md) |
