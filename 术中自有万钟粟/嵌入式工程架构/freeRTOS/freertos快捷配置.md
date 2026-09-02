# FreeRTOS 快捷配置

[← 返回 FreeRTOS](./MOC.md) | [← 主页](../../../index.md)

---

## cubemx快捷配置

![1783312869996](image/MOC/1783312869996.png)

![1783312905304](image/MOC/1783312905304.png)

这里选 TIM1 是因为 SysTick 是预留给 FreeRTOS 的，如果 HAL 库也用 SysTick 会有冲突，无法延时。
