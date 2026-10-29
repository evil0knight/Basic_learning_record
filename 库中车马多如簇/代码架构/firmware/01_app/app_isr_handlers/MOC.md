# app_isr_handlers

[← firmware](../../MOC.md) | [← 模块总览](../../../MOC.md) | [← 主页](../../../../../index.md)

[Handler 模板](../../05_bsp/bsp_xxx/handler/MOC.md) | [Driver 模板](../../05_bsp/bsp_xxx/driver/MOC.md) | [Adapter 示例](../../04_platform/platform_bsp/bsp_adapter/bsp_adapter_port_xxx.c)

---

## 关键文件

- [user_isr_handlers.h](./user_isr_handlers.h) / [user_isr_handlers.c](./user_isr_handlers.c)：HAL 回调入口模板，按实际外设包含 Platform 头文件并直接调用事件处理接口，不保存全局中断回调指针。
- [Platform Adapter](../../04_platform/platform_bsp/bsp_adapter/bsp_adapter_port_xxx.c)：组装平台接口、初始化设备，并管理平台资源与 BSP 的连接。
- [Handler](../../05_bsp/bsp_xxx/handler/bsp_xxx_handler.c)：绑定和初始化 Driver，组织设备状态及事件处理。
- [Driver](../../05_bsp/bsp_xxx/driver/bsp_xxx_driver.c)：提供可由 Platform 直接调用的设备中断处理接口。

## 调用顺序

```text
初始化：统一创建 OS 资源 -> Platform Adapter 组装接口 -> Handler/Driver 初始化

硬件中断 -> HAL 处理 -> App 中的 HAL 回调
    -> 检查引脚或外设句柄
    -> 直接调用 Platform core_xxx 事件处理接口
        -> 直接调用 Driver 的设备事件接口
        -> 或通过队列通知任务
```

本模板未指定具体外设，因此不预设 HAL 回调实现或 core_xxx 函数名。实际接入时先初始化资源和设备，再开启中断或 DMA 接收。中断上下文只执行必要操作；OSAL 队列发送须支持中断上下文。
