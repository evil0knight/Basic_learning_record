# UART(串口)

[← 返回 MOC](../MOC.md) | [← 主页](../../index.md)

> [下载串口驱动](https://pan.baidu.com/s/1OfEdpC5rkB4MYNhvWFjdkQ?pwd=4444)

UART 外设库：platform_mcu 层 Port（`core_usart_*`），HAL + DMA + 回调。

## 目录

| 目录 | 作用 |
| --- | --- |
| [UART_Port](./UART_Port/MOC.md) | `core_usart_*` 接口 + HAL 分发 + DMA + 回调 + 静态表 |
| [知识](./知识/MOC.md) | UART 协议、波特率、电平、收发写法等笔记 |

## 依赖

- HAL UART Driver（CubeMX 生成 `usart.h` / `usart.c`）
- 无 OS 依赖（Port 层裸机 / OS 通用）
