# platform_bsp

[返回 firmware](../../MOC.md) | [返回代码架构](../../../../MOC.md) | [返回主页](../../../../../index.md)

## 关键文件

- [bsp_wrapper_xxx.h](./bsp_wrapper/bsp_wrapper_xxx.h) / [bsp_wrapper_xxx.c](./bsp_wrapper/bsp_wrapper_xxx.c)：面向上层提供稳定的 XXX 设备接口，完成参数检查、状态转换和对外操作；关键函数为 `bsp_wrapper_xxx_register()`、`bsp_wrapper_xxx_init()` 和 `bsp_wrapper_xxx_control()`。
- [bsp_adapter_port_xxx.h](./bsp_adapter/bsp_adapter_port_xxx.h) / [bsp_adapter_port_xxx.c](./bsp_adapter/bsp_adapter_port_xxx.c)：组装平台资源和 BSP Handler，构造 Wrapper 所需的操作表，并在 `bsp_adapter_xxx_register()` 中完成挂载。

## 调用顺序

```text
平台初始化
    -> bsp_adapter_xxx_register()
    -> bsp_wrapper_xxx_register()
    -> bsp_wrapper_xxx_init()
    -> Handler 初始化
    -> Driver 初始化
    -> 上层调用 bsp_wrapper_xxx_control()
    -> Adapter
    -> Handler
    -> Driver
```
