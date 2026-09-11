# bsp_handler

[← 代码架构库](../../../../MOC.md) | [← 嵌入式工程架构](../../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../../MOC.md) | [← 主页](../../../../../../index.md)

---

管理 Driver 实例、设备状态、消息和统一控制入口，所需 OS 能力通过接口注入。

| 文件                                  | 作用                               |
| ------------------------------------- | ---------------------------------- |
| [bsp_xxx_handler.h](./bsp_xxx_handler.h) | 可复用：Handler对象和管理接口      |
| [bsp_xxx_handler.c](./bsp_xxx_handler.c) | 可复用：Driver注册、索引和统一控制 |

## Handler层职责

1. 定义 Handler 对象，保存 `driver_group`、注册数量和临界区接口。
2. 使用 `bsp_xxx_handler_init()` 注入临界区等依赖。
3. 使用 `bsp_xxx_handler_register()` 将 Driver 挂载到 `driver_group[index]`，注册过程由临界区保护。
4. `bsp_xxx_handler_control()` 根据 `index` 找到 Driver，再调用 `bsp_xxx_driver_control()`。
5. 需要异步处理时，再向 Handler 注入队列、互斥量、任务和延时接口，由任务接收消息并控制 Driver。

## 为什么要 Handler 层（以异步温湿度 IIC 为例）

用 AHT21 / DHT11（都经 IIC 读温湿度）这类**要等待、要排队**的异步外设，说明 App/Wrapper **不该直接同步调 Driver**，中间要有一层 Handler：

1. **复用（对上接口一致）**：AHT21、DHT11 都只是"读温湿度"，Driver 不同，但 Handler 的 register / control 对上接口一样 → 换器件只换挂载的 Driver，不动上层逻辑。
2. **非阻塞、让出 CPU（实时性）**：没有 Handler 时，App 直接同步等 Driver 是 CPU 空转等延时（如读温湿度前要等 ~80ms），RTOS 下实时性没保证。Handler 注入队列 + 线程 + OS delay：发起读取后阻塞等待、期间让出 CPU 干别的；数据就绪经回调传回 App。
3. **串行互斥（防并发打断）**：单任务从队列取请求**串行**处理 = 天然互斥——两个任务同时读同一 IIC，第二个不会把第一个的传输打断。
4. **批处理 / 吞吐**：多个请求进队列排队，Handler 按窗口批量响应（如 5ms 窗口内 2.5ms 处理完一批丢出），减少逐次往返。
5. **容灾 / 错误集中**：IIC 出错时的错误处理统一收敛在 Handler，不会散落进各处业务逻辑。
6. **线程 = CPU 时间片载体 → 并发**：Handler 内跑一个线程（任务），靠"阻塞 / 就绪"调度把 CPU 时间用满——等 80ms 时切出去干别的。严格说线程是**载体**，真正把 CPU 利用率做高的是 RTOS 的调度，不是线程本身。
7. **抽象 / 屏蔽底层**：向上隐藏读温湿度的具体时序、寄存器过程，App 只调接口取数，不用知道底层细节。
8. **临界保护初始化**：注册 + IIC 初始化这类关键段用**互斥量 / 临界区**保护、不被切走，而不是靠关中断死等。
9. **数据新鲜度缓存**：Handler 内部维护最近一次温度值 + 新鲜度窗口（如 30ms 内再次请求直接回缓存，不再发 IIC），省重复读取。

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
