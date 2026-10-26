# frimware_bare

[返回代码架构](../MOC.md) | [返回主页](../../../index.md)

## 关键文件

- [main_frame.c](08_core/Core/Src/main_frame.c)：裸机入口参考。使用时手动合入 CubeMX 生成的 `main.c`，合并后手动删除参考文件；合并前排除编译。
- [01_app](01_app/MOC.md)：事件队列、状态转移表和应用控制流程。
- [02_service](02_service/MOC.md)：工程 Service 由 `pf_action` 调用，并通过 Wrapper 操作设备。
- [03_config](03_config/MOC.md)：说明板级、功能、项目和版本配置。
- [04_platform](04_platform/MOC.md)： Adapter 和 Wrapper 的平台接口关系。
- [05_bsp](05_bsp/MOC.md)： Handler 和 Driver
- [06_middlewares](06_middlewares/MOC.md)：协议栈、文件系统和第三方组件的接入位置。
- [08_core](08_core/MOC.md)：启动代码、芯片初始化和裸机入口。
- [09_drivers](09_drivers/MOC.md)： CMSIS、芯片 HAL 和底层驱动依赖。
- [10_toolchain](10_toolchain/MOC.md)：编译器、链接脚本和构建配置。

## 调用顺序

```text
HAL 回调
    -> Driver 产生 EVENT_xxx
    -> 事件队列
    -> app_control_event_map()
    -> pf_action
    -> Service
    -> Wrapper
    -> Adapter
    -> Handler
    -> Driver
```
