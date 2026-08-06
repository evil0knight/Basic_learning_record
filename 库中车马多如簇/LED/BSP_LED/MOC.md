# BSP_LED

[← 返回模块库](../../MOC.md) | [← 主页](../../../index.md)

> 面向对象分层 LED 驱动架构：Driver → Handler → Platform Adapter → Wrapper → APP。

**架构说明**：[BSP,Platform写法 — 架构笔记](../../../术中自有万钟粟/嵌入式工程架构/BSP,Platform写法.md)

---

## 文件索引

| 层级                       | 文件                                            | 说明                                                                  |
| -------------------------- | ----------------------------------------------- | --------------------------------------------------------------------- |
| **Driver**           | [bsp_led_driver.h](./bsp_led_driver.h)             | LED 驱动层接口：对象结构体、实例化/控制函数声明                       |
|                            | [bsp_led_driver.c](./bsp_led_driver.c)             | LED 驱动层实现：inst、init、blink 控制逻辑                            |
| **Handler**          | [bsp_led_handle.h](./bsp_led_handle.h)             | LED 管理层接口：队列/互斥/线程抽象、注册/控制声明                     |
|                            | [bsp_led_handle.c](./bsp_led_handle.c)             | LED 管理层实现：inst、register、事件队列+线程处理                     |
| **Platform Adapter** | [bsp_adapter_port_led.h](./bsp_adapter_port_led.h) | 平台挂载入口：只公开 `drv_adapter_led_register()`                   |
|                            | [bsp_adapter_port_led.c](./bsp_adapter_port_led.c) | 对接 HAL/FreeRTOS，实例化并挂载具体 LED Driver，向 Wrapper 注册函数表 |
| **Wrapper**          | [bsp_wrapper_led.h](./bsp_wrapper_led.h)           | APP 稳定接口：初始化、按 index 控制、独立占空比类型                   |
|                            | [bsp_wrapper_led.c](./bsp_wrapper_led.c)           | 保存 Adapter 函数表，隔离 APP 与具体 BSP 接口                         |

---

## 调用流程

```
drv_adapter_led_register()         // Platform 初始化：把 Adapter 函数表挂载到 Wrapper
  └── drv_wrapper_led_register()

drv_wrapper_led_init()             // APP 初始化
  └── Adapter init 回调
      ├── handler_led_inst()       // 注入 OS 队列/互斥/线程接口
      ├── driver_led_inst(LED1)
      ├── handler_led_register()   // LED1 → led_group[0]
      ├── driver_led_inst(LED2)
      └── handler_led_register()   // LED2 → led_group[1]

drv_wrapper_led_control(index, ...)
  → Adapter control 回调
  → handler_led_control(index, ...)
  → led_group[index]
  → 队列 → handler_thread → driver_led_control → LED 闪烁
```

```c
drv_adapter_led_register();
drv_wrapper_led_init();

drv_wrapper_led_control(0U, 10U, 200U, LED_WRAPPER_PROPORTION_1_1);
drv_wrapper_led_control(1U, 10U, 200U, LED_WRAPPER_PROPORTION_1_1);
```
