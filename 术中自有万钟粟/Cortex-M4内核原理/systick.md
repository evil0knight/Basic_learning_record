# SysTick 定时器

[← 返回 Cortex-M4内核原理](./MOC.md) | [← 主页](../../index.md)

[← 返回 SystemView](../debug方法/Systemview.md)

---

## 作用:

1. 给RTOS当时基,触发硬件中断,保证任务调度,中断之后会触发pendsv调度([链表和调度](..\..\书中自有黄金屋\FreeRTOS内核实现与应用开发实战指南\链表和调度.md))就绪任务链表
2. SysTick 是包含在 ARM Cortex-M 核心内部的（和 NVIC 中断控制器绑在一起）
   这一点保证了RTOS可以适配不同芯片厂家的芯片,因为都遵守了ARM方案
   其他和普通[定时器](定时器.md)一样
