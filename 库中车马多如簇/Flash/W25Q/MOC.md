# W25Q

[← Flash](../MOC.md)

外部 Flash（W25Q64）器件层：事件驱动 Handler + 寄存器 Driver。

| 文件 | 作用 |
| --- | --- |
| [bsp_flash_handler.h](./bsp_flash_handler.h) / [bsp_flash_handler.c](./bsp_flash_handler.c) | Handler：事件队列 + 单任务串行处理，天然互斥 |
| [bsp_w25q64_driver.h](./bsp_w25q64_driver.h) / [bsp_w25q64_driver.c](./bsp_w25q64_driver.c) | Driver：寄存器操作（读/写/擦除/睡眠/唤醒），SPI 逐字节接口注入 |
| [bsp_w25q64_reg.h](./bsp_w25q64_reg.h) | 命令码 + 存储几何 |

Driver 的 SPI 逐字节接口（`spi_w25q64_interface_t`）由 [bsp_adapter_port_flash](../bsp_adapter_port_flash/bsp_adapter_port_flash.c) 用 `core_spi` 适配注入，Driver 本身不依赖具体 SPI 类型。

Handler 的 OS 接口（`flash_handler_os_interface_t`）由 Adapter 注入，Handler 本身不依赖具体 OS 类型。

接入目标工程见 [platform_bsp 使用说明](../../../代码架构/firmware/04_platform/platform_bsp/MOC.md)。
