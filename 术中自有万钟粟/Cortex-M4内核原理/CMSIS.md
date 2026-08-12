# CMSIS

[← 返回 Cortex-M4内核原理](./MOC.md) | [← 主页](../../index.md)

> Cortex Microcontroller Software Interface Standard — ARM 官方定义的内核硬件抽象层

---

## 定位

**向下吃透硬件**（直接操作内核寄存器与汇编），**向上吐出标准 API** 供 HAL 层使用。
C 语言开发所能触达的**最底层**，由 ARM 官方统一制定。

## 系统架构分层

| 层                    | 谁写        | 做什么                                   |
| --------------------- | ----------- | ---------------------------------------- |
| 应用层                | 你          | 业务逻辑（`main.c`）                     |
| 中间件层              | 第三方      | RTOS / 文件系统 / 图形库                 |
| 外设驱动层 (HAL/LL/标准库) | 芯片厂商 (ST/NXP) | GPIO、USART、CAN 等外设              |
| **CMSIS 层 (CORE)**   | **ARM 官方** | NVIC、SysTick、MPU 等内核外设          |
| 硬件层                | 芯片厂商    | Cortex-M 内核 + 外设物理电路             |

## 底层实现机制

- **内存映射**：结构体指针强转物理地址 → `SysTick->CTRL`
- **内联汇编**：内核汇编指令封装为 C 函数 → `__enable_irq()` = `CPSIE I`

## 四个模块

| 模块         | 抽象了什么                     | 给了 HAL 什么                                        |
| ------------ | ------------------------------ | ---------------------------------------------------- |
| CMSIS-CORE   | 内核寄存器（NVIC/SysTick/MPU） | 中断配置、系统定时器、内存保护的直操接口              |
| CMSIS-DSP    | DSP 硬件指令                   | FFT、矩阵运算、PID 控制等高级数学函数                |
| CMSIS-RTOS   | RTOS API                       | 隔离不同 RTOS 底层差异，统一 `osThreadNew` 等接口    |
| CMSIS-DAP    | JTAG/SWD 调试协议              | 统一 USB 烧录与调试接口                              |
