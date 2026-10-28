# UART(串口)

[← 返回 MOC](../MOC.md) | [← 主页](../../index.md)

[← OTA](../../术中自有万钟粟/OTA/MOC.md)

> [下载串口驱动](https://pan.baidu.com/s/1OfEdpC5rkB4MYNhvWFjdkQ?pwd=4444)

UART 外设库：`01_app` 承接 HAL 中断入口，`04_platform` 提供 Port（`core_usart_*`）、HAL + DMA + 回调分发。

## 目录

| 目录 | 作用 |
| --- | --- |
| [01_app/app_isr_handlers](./01_app/app_isr_handlers/user_isr_handlers.c) | HAL UART 中断回调入口，转发到 USART Port |
| [04_platform/platform_mcu/uart](./04_platform/platform_mcu/uart/MOC.md) | `core_usart_*` 接口 + HAL 分发 + DMA + 回调 + 静态表 |
| [知识](./知识/MOC.md) | UART 协议、波特率、电平、收发写法等笔记 |

## 中断调用链

```text
HAL_UARTEx_RxEventCallback()
    -> core_usart_irq_rx_event()
    -> g_pf_usart_rx_callbacks[instance]
    -> 用户注册的 RX 回调
```

## 依赖

- HAL UART Driver（CubeMX 生成 `usart.h` / `usart.c`）
- 无 OS 依赖（Port 层裸机 / OS 通用）
