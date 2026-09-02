# xxx_device

[← xxx](../MOC.md)

xxx 器件层：事件驱动 Handler + 寄存器 Driver。

| 文件 | 作用 |
| --- | --- |
| [bsp_xxx_handler.h](./bsp_xxx_handler.h) / [bsp_xxx_handler.c](./bsp_xxx_handler.c) | Handler：事件队列 + 单任务串行处理，天然互斥 |
| [bsp_xxx_driver.h](./bsp_xxx_driver.h) / [bsp_xxx_driver.c](./bsp_xxx_driver.c) | Driver：寄存器操作（读/写/命令），总线逐字节接口注入 |
| [bsp_xxx_reg.h](./bsp_xxx_reg.h) | 命令码 + 器件几何 |

Driver 的总线逐字节接口（`xxx_bus_interface_t`）由 [bsp_adapter_port_xxx](../bsp_adapter_port_xxx/bsp_adapter_port_xxx.c) 用总线 Port 适配注入，Driver 本身不依赖具体总线类型。

Handler 的 OS 接口（`xxx_handler_os_interface_t`）由 Adapter 注入，Handler 本身不依赖具体 OS 类型。

接入目标工程见 [platform_bsp 使用说明](../../../代码架构/firmware/04_platform/platform_bsp/MOC.md)。
