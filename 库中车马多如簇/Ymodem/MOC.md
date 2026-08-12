# Ymodem

| [← 模块总览](../MOC.md) | [← 主页](../../index.md) |
| --- | --- |

Ymodem 是基于串口的文件传输协议。该目录保存 STM32 Bootloader 接收固件所需的参考源码和 F411 Flash 适配层。

| 文件 | 内容 |
| --- | --- |
| [协议、移植与使用](../../术中自有万钟粟/OTA/bootloader/Ymodem传输协议.md) | F411 初始化、Flash 布局、调用和排错 |
| [ymodem.h](./ymodem.h) / [ymodem.c](./ymodem.c) | Ymodem 收发状态机 |
| [common.h](./common.h) / [common.c](./common.c) | 字符串工具和串口收发适配 |
| [Usart.c](../UART内含串口助手安装包/Usart/Usart.c) / [Usart.h](../UART内含串口助手安装包/Usart/Usart.h) | STM32F4 标准库 USART1 驱动 |
| [Flash 驱动](../flash/MOC.md) | F411 APP 区擦除与写入，Ymodem 工程需加入该目录 |

> 参考源码仍需按协议笔记补接收 CRC 校验和 APP 容量判断后再用于升级。
