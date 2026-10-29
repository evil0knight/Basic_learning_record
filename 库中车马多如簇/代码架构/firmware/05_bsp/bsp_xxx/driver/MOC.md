# bsp_xxx Driver

[← firmware](../../../MOC.md) | [← 模块总览](../../../../MOC.md) | [← 主页](../../../../../../index.md)

[Handler 说明](../handler/MOC.md) | [Platform BSP](../../../04_platform/platform_bsp/MOC.md)

---

## 目录职责

`bsp_xxx_driver` 是单个设备的硬件驱动层，负责寄存器、总线协议、数据转换和设备本身的中断处理。它通过接口表使用平台能力，不直接依赖具体 MCU、OS 或 Adapter 实现。

## 依赖方向

```text
Adapter 组装 pf_* 接口
    -> Driver bind/init
    -> Platform 直接转发设备中断事件
    -> Driver 运行硬件操作
    -> Handler/Wrapper 调用 Driver
```

Driver 不创建业务任务、消息队列或通用设备注册表。需要异步调度时由具体 Handler 管理。

## 接口表

接口表只保留设备实际需要的函数，例如：

```c
typedef struct
{
    bsp_xxx_status_t (*pf_init)(void *p_context);
    bsp_xxx_status_t (*pf_control)(void *p_context,
                                   uint32_t command,
                                   void *p_argument);
    bsp_xxx_status_t (*pf_critical_enter)(void);
    bsp_xxx_status_t (*pf_critical_exit)(void);
} bsp_xxx_port_ops_t;
```

Adapter 负责把 GPIO、IIC、SPI、DMA、临界区等实际函数填入接口表。Driver 只检查接口表和上下文，然后通过 `pf_*` 调用平台能力。

## 中断和 DMA 回调

Driver 公开 bsp_xxx_int_interrupt_callback() 和 bsp_xxx_dma_interrupt_callback()，由 Platform 在识别设备后直接调用。App 的 HAL 回调只调用 Platform 接口；Driver 不依赖 App 的全局回调指针。

初始化接口为 bsp_xxx_driver_init(p_driver)，不再接收回调注册函数。具体设备在事件入口检查对象和数据有效性，完成必要硬件处理；耗时业务交给任务。

## 编写规则

- 所有公开函数检查对象、接口表、上下文和函数指针。
- 初始化顺序固定为：参数检查、接口绑定、硬件复位/识别、设置已初始化状态。
- 控制命令和寄存器访问使用设备专用枚举、结构体和错误码，避免无意义的万能命令。
- 临界区只保护必须的寄存器或共享状态，及时退出并返回底层错误。
- Driver 失败时保持对象处于未初始化或错误状态，避免 Handler 误以为设备可用。

## 文件索引

- [Driver 头文件](./bsp_xxx_driver.h)
- [Driver 实现](./bsp_xxx_driver.c)
- [Handler 模板](../handler/MOC.md)
- [Adapter 模板](../../../04_platform/platform_bsp/bsp_adapter/bsp_adapter_port_xxx.c)
- [ISR 回调入口](../../../01_app/app_isr_handlers/MOC.md)
