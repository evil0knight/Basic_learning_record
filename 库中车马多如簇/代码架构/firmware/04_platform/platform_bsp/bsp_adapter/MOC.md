# bsp_adapter

[← platform_bsp](../MOC.md) | [← firmware](../../../MOC.md) | [← 模块总览](../../../../MOC.md) | [← 主页](../../../../../../index.md)

[Driver 说明](../../../05_bsp/bsp_xxx/driver/MOC.md) | [Handler 说明](../../../05_bsp/bsp_xxx/handler/MOC.md)

---

## 目录职责

Adapter 是平台和 BSP 设备之间的组装层。它把 GPIO、IIC、SPI、DMA、临界区和 OS 函数组装成 Driver 的 `pf_*` 接口表，创建本设备的 Driver/Handler 实例，再把 Handler 注册到对应 Wrapper。

## 组装顺序

1. 实现平台相关的 `pf_*` 函数和上下文。
2. 填充本设备的接口表。
3. 调用 Handler 初始化，由 Handler 完成 Driver bind/init 和回调挂载。
4. 把 Handler 的控制/读写接口包装成 Wrapper 操作表。
5. 在系统启动阶段调用 `bsp_adapter_xxx_register()`。

Adapter 不实现设备协议、不解析中断事件、不创建所有设备共用的注册器。设备业务逻辑应放在具体 Handler，寄存器和总线操作应放在具体 Driver。

## 文件索引

- [Adapter 头文件](./bsp_adapter_port_xxx.h)
- [Adapter 实现](./bsp_adapter_port_xxx.c)
- [Platform BSP 总览](../MOC.md)
- [Handler 模板](../../../05_bsp/bsp_xxx/handler/MOC.md)
- [Driver 模板](../../../05_bsp/bsp_xxx/driver/MOC.md)
