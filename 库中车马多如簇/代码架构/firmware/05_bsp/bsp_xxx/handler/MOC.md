# bsp_xxx Handler

[← firmware](../../../MOC.md) | [← 模块总览](../../../../MOC.md) | [← 主页](../../../../../../index.md)

[Driver 说明](../driver/MOC.md) | [Platform BSP](../../../04_platform/platform_bsp/MOC.md)

---

## 目录职责

`bsp_xxx_handler` 是一个具体设备的业务协调层模板。它负责组织 Driver、设备状态、事件队列、任务和上层回调；它不负责实现寄存器协议，也不负责把所有设备放进一张通用注册表。

不同设备 Handler 按设备功能组织运动数据、采样事件、存储完成事件或播放队列。使用模板时，先确定设备工作模型，再确定 Handler 的成员和接口。

## 基础职责边界

```text
HAL/MCU ISR
    -> app_isr_handlers 中的设备回调指针
    -> 设备 Handler 提供的挂载函数
    -> Driver 的具体中断处理函数或 Handler 的事件投递函数
    -> Driver 完成寄存器/协议操作
    -> Handler 更新状态、队列或上层回调
    -> Adapter/Wrapper 对外提供统一调用入口
```

| 层 | 职责 | 边界 |
| --- | --- | --- |
| `app_isr_handlers` | 保存 ISR 回调指针，提供 HAL 回调入口 | 执行回调转发 |
| Handler | 按设备组织初始化、事件、状态、缓存、任务和回调挂载 | 通过 Driver 使用硬件操作 |
| Driver | 使用接口表完成寄存器、总线、协议和硬件处理 | 提供设备硬件操作 |
| Adapter | 组装 `pf_*` 接口、创建本设备实例、调用 Handler 初始化 | 管理平台与 BSP 的连接 |
| Wrapper | 为上层暴露稳定的设备接口 | 通过 Adapter 访问 BSP |

## 按设备类型选择 Handler 结构

### 1. 基础同步设备

适用于 GPIO 扩展、简单传感器、同步存储器等。Handler 只保存 Driver 指针，并提供初始化和控制转发：

```c
typedef struct
{
    bsp_xxx_driver_t *p_driver;
} bsp_xxx_handler_t;
```

`bsp_xxx_handler_init()` 完成 Driver 绑定、Driver 初始化和必要的回调挂载；`bsp_xxx_handler_control()` 只做参数、索引和状态检查，然后调用 Driver。单设备默认只接受 `index == 0`，仅在存在多个物理实例时增加实例索引表。

### 2. 中断或 DMA 设备

适用于 MPU、触摸、串口 DMA、带数据就绪脚的传感器等。Handler 提供该设备自己的回调挂载函数，Driver 在初始化时把具体回调传入挂载函数：

```c
status = bsp_xxx_driver_init(p_driver,
                             bsp_xxx_register_int_callback,
                             bsp_xxx_register_dma_callback);
```

挂载函数只负责把回调赋给 `app_isr_handlers` 中对应设备的回调指针：

```c
static bsp_xxx_status_t bsp_xxx_register_int_callback(
    bsp_xxx_isr_callback_t pf_callback)
{
    if (pf_callback == NULL)
    {
        return BSP_XXX_ERR_ARGUMENT;
    }

    pf_xxx_int_interrupt_callback = pf_callback;
    return BSP_XXX_OK;
}
```

HAL 中断到来后，`app_isr_handlers` 只转发已经挂载的回调；回调指向 Driver 的具体处理函数。中断上下文中只做清中断、取必要数据或投递事件，耗时处理放到 Handler 任务中。

根据实际中断源保留对应的挂载函数。

### 3. 异步队列和任务设备

适用于温湿度采样、触摸事件、Flash 异步读写、Speaker 播放等。Handler 自己定义事件类型、队列、任务和完成回调：

```c
typedef enum
{
    BSP_XXX_EVENT_DATA_READY,
    BSP_XXX_EVENT_TRANSFER_DONE,
    BSP_XXX_EVENT_ERROR
} bsp_xxx_event_type_t;

typedef struct
{
    bsp_xxx_event_type_t type;
    uint32_t value;
} bsp_xxx_event_t;
```

中断回调只构造最小事件并投递队列；Handler 任务从队列取事件，调用 Driver 读取数据、更新状态，再调用上层完成回调。队列句柄、任务句柄和 OS 函数指针属于本设备 Handler，由具体 Handler 管理。

### 4. 带缓存或状态机的设备

适用于触摸去抖、运动数据融合、音频播放和分段 Flash 操作。Handler 增加设备专属状态机和缓存，例如：

```c
typedef enum
{
    BSP_XXX_STATE_RESET,
    BSP_XXX_STATE_READY,
    BSP_XXX_STATE_BUSY,
    BSP_XXX_STATE_ERROR
} bsp_xxx_state_t;
```

状态迁移、缓存所有权和超时处理写在该设备 Handler 中；Driver 只提供一次硬件操作和状态查询。初始化失败时应清理已创建的队列、任务或回调绑定，并返回明确错误码。

### 5. 多实例设备

只有同一设备确实存在多个物理实例时，才在具体 Handler 中增加数组或实例索引：

```c
typedef struct
{
    bsp_xxx_driver_t *p_driver;
    uint32_t instance_count;
} bsp_xxx_handler_t;
```

索引范围、每个实例的端口接口、回调和资源由该设备自行定义。实例索引仅用于确有多个物理实例的设备。

## 初始化和调用顺序

1. Adapter 组装 GPIO、IIC、SPI、DMA、临界区等 `pf_*` 接口表。
2. Adapter 创建本设备 Driver/Handler 对象，并调用 `bsp_xxx_handler_init()`。
3. Handler 调用 Driver bind/init，并把 Driver 回调挂到对应 ISR 回调指针。
4. HAL ISR 到来时，`app_isr_handlers` 转发回调。
5. Handler 根据设备类型直接处理，或投递队列交给任务处理。
6. Wrapper 通过 Adapter 调用 Handler 的控制、读写或专用接口。

## 从模板复制到真实设备

- 把 `xxx` 替换为真实设备名，重新设计状态码、命令和事件类型。
- 先确定设备是同步、异步、中断/DMA、缓存状态机还是多实例，再裁剪 Handler 成员。
- 只声明设备实际需要的 `pf_*`、回调挂载函数、队列和任务。
- 为每个中断源提供清晰的设备专用回调挂载函数；挂载函数按设备和中断源分别定义。
- 在 Adapter 中完成底层接口组装，在 Driver 中实现硬件操作，在 Wrapper 中保持上层调用稳定。

## 文件索引

- [Handler 头文件](./bsp_xxx_handler.h)
- [Handler 实现](./bsp_xxx_handler.c)
- [Driver 模板](../driver/MOC.md)
- [Adapter 模板](../../../04_platform/platform_bsp/bsp_adapter/bsp_adapter_port_xxx.c)
- [ISR 回调入口](../../../01_app/app_isr_handlers/MOC.md)
