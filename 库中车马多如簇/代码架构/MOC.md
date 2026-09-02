# 代码架构库

[← 模块总览](../MOC.md) | [← 嵌入式工程架构](../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 主页](../../index.md)

---

下载同目录下的`firmware`文件夹

| 根目录   | 1级                                             | 2级                                                        | 3级                                                                           |
| -------- | ----------------------------------------------- | ---------------------------------------------------------- | ----------------------------------------------------------------------------- |
| firmware | 01_app                                          | [app_init](./firmware/01_app/app_init/MOC.md)                 |                                                                               |
|          |                                                 | [app_isr_handlers](./firmware/01_app/app_isr_handlers/MOC.md) |                                                                               |
|          |                                                 | [ui](./firmware/01_app/ui/MOC.md)                             |                                                                               |
|          | [02_service](./firmware/02_service/MOC.md)         |                                                            |                                                                               |
|          | [03_config](./firmware/03_config/MOC.md)           |                                                            |                                                                               |
|          | 04_platform                                     | platform_bsp                                               | [bsp_wrapper](./firmware/04_platform/platform_bsp/bsp_wrapper/MOC.md)            |
|          |                                                 |                                                            | [bsp_adapter](./firmware/04_platform/platform_bsp/bsp_adapter/MOC.md)            |
|          |                                                 | [platform_mcu](./firmware/04_platform/platform_mcu/MOC.md)    | [GPIO Port](./firmware/04_platform/platform_mcu/gpio/MOC.md)                     |
|          |                                                 | platform_os                                                | [inc](./firmware/04_platform/platform_os/inc/MOC.md)                             |
|          |                                                 |                                                            | [os_wrapper](./firmware/04_platform/platform_os/os_wrapper/MOC.md)               |
|          |                                                 |                                                            | [os_implementation](./firmware/04_platform/platform_os/os_implementation/MOC.md) |
|          | 05_bsp                                          | bsp_xxx                                                    | [driver](./firmware/05_bsp/bsp_xxx/driver/MOC.md)                                |
|          |                                                 |                                                            | [handler](./firmware/05_bsp/bsp_xxx/handler/MOC.md)                              |
|          | [06_middlewares](./firmware/06_middlewares/MOC.md) |                                                            |                                                                               |
|          | [07_os](./firmware/07_os/MOC.md)                   |                                                            |                                                                               |
|          | [08_core](./firmware/08_core/MOC.md)               |                                                            |                                                                               |
|          | [09_drivers](./firmware/09_drivers/MOC.md)         |                                                            |                                                                               |
|          | [10_toolchain](./firmware/10_toolchain/MOC.md)     |                                                            |                                                                               |
|          | [11_utils](./firmware/11_utils/MOC.md)             |                                                            |                                                                               |
