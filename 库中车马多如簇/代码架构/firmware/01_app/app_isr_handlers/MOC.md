# app_isr_handlers

[← firmware](../../MOC.md) | [← 模块总览](../../../MOC.md) | [← 主页](../../../../../index.md)

[Handler 模板](../../05_bsp/bsp_xxx/handler/MOC.md) | [Driver 模板](../../05_bsp/bsp_xxx/driver/MOC.md) | [Adapter 示例](../../04_platform/platform_bsp/bsp_adapter/bsp_adapter_port_xxx.c)

---

## 关键文件

- [user_isr_handlers.h](./user_isr_handlers.h) / [user_isr_handlers.c](./user_isr_handlers.c)：声明和定义 `pf_pin_interrupt_callback`、`pf_dma_interrupt_callback`、`pf_int_interrupt_callback`，为 HAL 回调提供设备中断回调指针。
- [bsp_adapter_port_xxx.c](../../04_platform/platform_bsp/bsp_adapter/bsp_adapter_port_xxx.c)：组装平台 `pf_*` 接口并启动具体设备 Handler 初始化，连接平台资源与 BSP 设备。
- [bsp_xxx_handler.c](../../05_bsp/bsp_xxx/handler/bsp_xxx_handler.c)：提供中断回调注册函数，将 Driver 回调地址写入 ISR 回调指针，并完成 Handler 初始化。
- [bsp_xxx_driver.c](../../05_bsp/bsp_xxx/driver/bsp_xxx_driver.c)：实现具体的引脚中断、DMA 完成或设备中断处理函数，完成清中断、寄存器读取和数据处理。

## 调用顺序

~~~text
系统初始化
 └─ Platform Adapter
     ├─ 组装 pf_* 底层接口
     └─ bsp_xxx_handler_init()
         ├─ Handler 绑定 Driver
         ├─ Driver 初始化
         ├─ Handler 提供回调注册函数
         └─ Driver 回调地址写入 ISR 指针
             ├─ pf_pin_interrupt_callback
             ├─ pf_dma_interrupt_callback
             └─ pf_int_interrupt_callback

硬件中断
 └─ MCU IRQHandler
     └─ HAL 中断处理函数
         └─ HAL 回调
             ├─ 检查 GPIO 引脚或外设句柄
             └─ 调用对应 ISR 回调指针
                 └─ BSP Driver 中断处理函数
                     └─ Handler 队列、状态或上层回调
~~~