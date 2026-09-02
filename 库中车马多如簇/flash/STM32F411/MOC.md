# STM32F411 Flash

[← Flash](../MOC.md)

内部 Flash（STM32F411 片内），同步纯函数，独立于外部 Flash 的异步事件驱动架构。

| 文件 | 作用 |
| --- | --- |
| [flash.h](./flash.h) / [flash.c](./flash.c) | STM32F411 Flash Driver：扇区擦除 + 字编程 |
| [stm32f411_flash_handler.h](./stm32f411_flash_handler.h) / [stm32f411_flash_handler.c](./stm32f411_flash_handler.c) | 范围读写 + 按字编程 + 回读校验 |

内部 Flash 无状态、单次操作原子，直接同步调用即可（如 OTA 标志区读写），不参与外部 Flash 的事件驱动链路。
