# firmware

[返回代码架构](../MOC.md) | [返回主页](../../../index.md)

## 关键目录

- [01_app](./01_app/app_init/MOC.md)：应用初始化、ISR 回调和用户任务。
- [02_service](./02_service/MOC.md)：业务服务和功能流程。
- [03_config](./03_config/MOC.md)：板级、功能、RTOS、CubeMX 和版本配置。
- [04_platform](./04_platform/platform_bsp/MOC.md)：Platform MCU、BSP Adapter 和 Wrapper。
- [05_bsp](./05_bsp/MOC.md)：设备 Handler 和 Driver。
- [06_middlewares](./06_middlewares/MOC.md)：算法、通信、文件系统、图形库和 Vendor 组件。
- [07_os](./07_os/MOC.md)：OS 适配和系统资源。
- [08_core](./08_core/MOC.md)：芯片启动、系统初始化和中断入口。
- [09_drivers](./09_drivers/MOC.md)：CMSIS、芯片 HAL 和底层驱动包。
- [10_toolchain](./10_toolchain/MOC.md)：编译器、Keil 工程和链接脚本。

## 调用顺序

```text
Core 启动
    -> Platform MCU / OS 初始化
    -> BSP Driver / Handler
    -> Service
    -> App
```
