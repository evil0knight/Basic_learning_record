# CAN 工程代码

[← CAN 总览](../MOC.md) | [← 主页](../../../index.md)

---

## 通用分层版

面向多路经典 CAN / CAN FD，实例表绑定句柄，RX 两个 FIFO 独立注册回调，TX 和错误事件由 App 层转发。

```text
通用分层版/
├── 01_app/app_isr_handlers/
│   ├── user_isr_handlers.c
│   └── user_isr_handlers.h
└── 04_platform/platform_mcu/can/
    ├── inc/
    │   ├── can_port.h
    │   └── can_port_config.h
    └── src/
        └── can_port.c
```

| 文件 | 用途 |
| --- | --- |
| [can_port.h](./通用分层版/04_platform/platform_mcu/can/inc/can_port.h) | 帧、状态、实例、同步/异步 API 与回调类型 |
| [can_port_config.h](./通用分层版/04_platform/platform_mcu/can/inc/can_port_config.h) | 实例表，绑定 hfdcan1/hfdcan2 或 hcan1/hcan2 |
| [can_port.c](./通用分层版/04_platform/platform_mcu/can/src/can_port.c) | HAL 收发、过滤器和 IRQ 分发 |
| [user_isr_handlers.c](./通用分层版/01_app/app_isr_handlers/user_isr_handlers.c) / [头文件](./通用分层版/01_app/app_isr_handlers/user_isr_handlers.h) | 按后端选择 HAL FDCAN / CAN 回调覆盖入口 |

配置步骤见 [移植指南](../移植指南.md)。

## FOC简化版

单路 hfdcan1 或 hcan1 的直接封装，不建实例表，不提供分层 ISR 文件。

```text
FOC简化版/04_platform/platform_mcu/can/
├── inc/core_can.h
└── src/core_can.c
```

| 文件 | 用途 |
| --- | --- |
| [core_can.h](./FOC简化版/04_platform/platform_mcu/can/inc/core_can.h) | 单路经典 CAN / CAN FD 接口 |
| [core_can.c](./FOC简化版/04_platform/platform_mcu/can/src/core_can.c) | 同步/异步发送、FIFO 轮询、过滤器和通知开关 |

HAL 回调由用户工程处理；默认 FDCAN 后端，也可选择 bxCAN。高实时 ISR 内仅使用异步发送，禁止同步等待。

## 两版差异

| 对比项 | 通用分层版 | FOC简化版 |
| --- | --- | --- |
| 硬件绑定 | 静态实例表，可扩展 CAN2 | 直接使用 hfdcan1 / hcan1 |
| 回调 | FIFO0/FIFO1、TX、错误独立注册 | 用户工程自行实现 HAL 回调 |
| RX 通知 | 注册 RX 回调自动开关，也可显式开关 | 显式 enable/disable |
| 发送 | 同步 + 异步 | 同步 + 异步 |
| 依赖 | STM32 FDCAN / CAN HAL | STM32 FDCAN / CAN HAL |

两版只能选择一版加入工程，符号同名。均支持经典帧；FDCAN 后端支持 64 字节 FD 与 BRS。均不提供 RTOS 依赖、软件队列或线程锁。

