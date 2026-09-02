# bsp_handler

[← 代码架构库](../../../../MOC.md) | [← 嵌入式工程架构](../../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../../MOC.md) | [← 主页](../../../../../../index.md)

---

管理 Driver 实例、设备状态、消息和统一控制入口，所需 OS 能力通过接口注入。

| 文件 | 作用 |
| --- | --- |
| [bsp_xxx_handler.h](./bsp_xxx_handler.h) | 可复用：Handler对象和管理接口 |
| [bsp_xxx_handler.c](./bsp_xxx_handler.c) | 可复用：Driver注册、索引和统一控制 |

## Handler层职责

1. 定义 Handler 对象，保存 `driver_group`、注册数量和临界区接口。
2. 使用 `bsp_xxx_handler_init()` 注入临界区等依赖。
3. 使用 `bsp_xxx_handler_register()` 将 Driver 挂载到 `driver_group[index]`，注册过程由临界区保护。
4. `bsp_xxx_handler_control()` 根据 `index` 找到 Driver，再调用 `bsp_xxx_driver_control()`。
5. 需要异步处理时，再向 Handler 注入队列、互斥量、任务和延时接口，由任务接收消息并控制 Driver。

## bsp_handler配置:

1. 复制 `bsp_xxx_handler.h/.c`，将 `xxx` 改为模块名。
2. Handler 保存 Driver 实例和设备状态：

```c
int32_t bsp_xxx_handler_init(void);
int32_t bsp_xxx_handler_control(uint32_t index,
                                uint32_t command, void *argument);
```

3. 需要异步处理时，在 Handler 中使用 OSAL 队列、互斥量或任务。
4. Wrapper 通过 Adapter 调用 Handler，Handler 再调用 Driver。
