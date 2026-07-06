# BSP_LED

[← 返回 LED](../MOC.md) | [← 主页](../../../index.md)

> 面向对象分层 LED 驱动架构：Driver → Handler → System 三层解耦。

**架构说明**：[面向对象驱动 — 架构笔记](../../../术中自有万钟粟/嵌软高手/面向对象驱动.md)

---

## 文件索引

| 层级 | 文件 | 说明 |
| ---- | ---- | ---- |
| **Driver** | [bsp_led_driver.h](./bsp_led_driver.h) | LED 驱动层接口：对象结构体、实例化/控制函数声明 |
| | [bsp_led_driver.c](./bsp_led_driver.c) | LED 驱动层实现：inst、init、blink 控制逻辑 |
| **Handler** | [bsp_led_handle.h](./bsp_led_handle.h) | LED 管理层接口：队列/互斥/线程抽象、注册/控制声明 |
| | [bsp_led_handle.c](./bsp_led_handle.c) | LED 管理层实现：inst、register、事件队列+线程处理 |
| **System** | [system_adaption.h](./system_adaption.h) | 系统适配层接口：FreeRTOS/CMSIS 依赖与全局实例声明 |
| | [system_adaption.c](./system_adaption.c) | 系统适配层实现：硬件回调绑定、OS 接口对接、初始化入口 |

---

## 调用流程

```
system_adaption_init()
  ├── handler_led_inst()       // 注入 OS 队列/互斥/线程接口
  ├── driver_led_inst()        // 注入 LED 操作/时基/延时接口
  └── handler_led_register()   // driver 挂载到 handler 统一管理

handler_led_control()          // 外部通过 handler 发消息
  → 队列 → handler_thread → driver_led_control → LED 闪烁
```
