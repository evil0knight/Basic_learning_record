# 05_bsp

[返回 firmware](../MOC.md) | [返回代码架构](../../MOC.md) | [返回主页](../../../../index.md)

## 关键文件

- [bsp_xxx_handler.h](./bsp_xxx/handler/bsp_xxx_handler.h) / [bsp_xxx_handler.c](./bsp_xxx/handler/bsp_xxx_handler.c)：保存 Driver 实例和平台接口，负责 Handler 初始化、回调挂载、设备控制转发和事件处理；关键函数为 `bsp_xxx_handler_init()` 和 `bsp_xxx_handler_control()`。
- [bsp_xxx_driver.h](./bsp_xxx/driver/bsp_xxx_driver.h) / [bsp_xxx_driver.c](./bsp_xxx/driver/bsp_xxx_driver.c)：实现 XXX 设备的寄存器操作、平台接口调用和具体中断回调；关键函数为 `bsp_xxx_driver_init()`、`bsp_xxx_driver_control()` 和设备中断回调函数。

## 调用顺序

```text
Adapter 挂载平台接口
    -> Handler 初始化 Driver
    -> Handler 初始化并挂载中断回调
    -> HAL 中断回调
    -> Driver 中断处理
    -> Handler 转发设备操作
    -> Wrapper 对外提供设备接口
```

