# I2C Bus / Port

[← MCU 平台](../MOC.md) | [← BSP](../../../05_bsp/MOC.md)

> 先配置总线槽位，再用 Port 接口访问器件。

---

| 文件                                                                                      | 作用                                           |
| ----------------------------------------------------------------------------------------- | ---------------------------------------------- |
| [i2c_port_types.h](./bus/inc/i2c_port_types.h)                                               | 状态码、逻辑槽位号、完成回调和错误回调类型     |
| [i2c_bus.h](./bus/inc/i2c_bus.h) / [i2c_bus.c](./bus/src/i2c_bus.c)                             | Bus：协议、HAL 分发、DMA、`busy`、中断分发   |
| [i2c_bus_config.h](./bus/inc/i2c_bus_config.h)                                               | 硬件/软件 I2C 槽位和板级资源配置               |
| [soft_i2c.h](./bus/inc/soft_i2c.h) / [soft_i2c.c](./bus/src/soft_i2c.c)                         | 软件 I2C 内部接口和 START、STOP、ACK/NACK 时序 |
| [i2c_port_bare.h](./port/bare/i2c_port_bare.h) / [i2c_port_bare.c](./port/bare/i2c_port_bare.c) | 裸机 Port：请求队列、超时、软件槽位执行        |
| [i2c_port_os.h](./port/os/i2c_port_os.h) / [i2c_port_os.c](./port/os/i2c_port_os.c)             | RTOS Port：每总线注册 mutex/sema，内部加锁     |
| [user_isr_handlers.c](../01_app/app_isr_handlers/user_isr_handlers.c)                        | HAL I2C 中断入口，转发到 Bus                   |
| [GPIO Port](../gpio/MOC.md)                                                                  | 软件 I2C 所需的直接 GPIO 接口                  |

## 分层

| 层               | 职责                                                                  | 不做                             |
| ---------------- | --------------------------------------------------------------------- | -------------------------------- |
| Bus              | HAL、DMA、`busy`、回调、IRQ、阻塞/异步传输                          | 不创建 OS 资源、不加锁、不判超时 |
| 裸机 Port        | 请求队列、`platform_critical_enter/exit`、`core_time_get()`、超时 | 不碰 HAL 细节                    |
| RTOS Port        | 每总线注册的 mutex/sema、`osal_sema_take/give`、超时                | 不创建资源                       |
| Service / Driver | 业务逻辑，调 Port                                                     | 不碰 I2C 协议、不碰锁            |

Bus 只写一份，裸机和 RTOS 各选一个 Port 复制。

## 三种传输方式

两个维度：**搬运方式** × **结果怎么拿**。

|                                         | 调用方在本函数里拿到结果 | 结果在别处拿 |
| --------------------------------------- | ------------------------ | ------------ |
| **普通 HAL**（CPU 在 HAL 内轮询） | ① 普通 HAL              | ✗ 无意义    |
| **DMA**                           | ② 同步                  | ③ 异步      |

| 编号 | 裸机                               | RTOS                            | 结果怎么拿                                                       |
| ---- | ---------------------------------- | ------------------------------- | ---------------------------------------------------------------- |
| ①   | `core_i2c_mem_read/write()`      | 同左                            | 裸机临界区 / RTOS mutex，HAL 内部轮询，**返回即完成**            |
| ②   | `core_i2c_mem_read/write_sync()` | 同左                            | 裸机：**非阻塞轮询**，首次提交返回 PENDING，之后查到 done 为止；RTOS：`osal_sema_take` 阻塞等 |
| ③   | `core_i2c_submit(&req)`          | `core_i2c_read/write_async()` | 回调 + 信号量，都在调用方那一侧                                  |

裸机 `_sync` 与 RTOS `_sync` 的**调用形态不同**：

```c
/* 裸机：每次调用立刻返回，靠反复调用来推进 */
while (1)
{
    st = core_i2c_mem_read_sync(...);
    if (CORE_I2C_OK == st) { 用数据; }
}

/* RTOS：一次调用阻塞到完成（或超时） */
st = core_i2c_mem_read_sync(...);
```

## 状态集中管理

| 层        | 表                                 | 每个槽位里装什么                                                                                             |
| --------- | ---------------------------------- | ------------------------------------------------------------------------------------------------------------ |
| Bus       | `g_i2c_slots[CORE_I2C_PORT_MAX]` | `type` / `busy` / `pf_rx` / `pf_tx` / `pf_error` / `config` / `dma`                            |
| 裸机 Port | `g_i2c_bare[CORE_I2C_PORT_MAX]`  | 请求队列（存指针）、在飞请求指针、abort 收尾状态、错误码                                                       |
| RTOS Port | `g_i2c_os[CORE_I2C_PORT_MAX]`    | `mutex` / `sema` / `status` / `error_code` / `device_address_7bit` / `pf_done` / `p_user_data` |

每笔请求自己带结果，队列里排多笔也不会互相覆盖：

```c
typedef struct
{
    ...传输参数（port / 设备地址 / 寄存器 / p_data / size / timeout_ms）...
    core_i2c_done_callback_t     pf_done;    // 可为 NULL，纯轮询就不挂
    void                        *p_user_data;

    volatile uint8_t              done;      // Port 完成时写 1，调用方读完清 0
    volatile en_core_i2c_status_t status;    // 本笔的完成状态
} core_i2c_request_t;
```

裸机 Port 的结构体：

```c
typedef struct
{
    core_i2c_request_t *req[CORE_I2C_QUEUE_DEPTH];  // 待处理请求（存指针）
    volatile uint8_t    head;                       // 队首下标
    volatile uint8_t    count;                      // 待处理个数
    core_i2c_request_t *cur;                        // 在飞请求（存指针）
    volatile uint8_t    cur_busy;                   // 在飞标志
    volatile uint8_t    skip_done;                  // 丢弃 abort 迟到中断
    uint32_t            cur_tick;                   // 在飞起点
    volatile uint32_t   last_error;                 // HAL 原始错误码
} st_i2c_port_bare_t;
```

队列存**指针**而不是拷贝，否则完成中断写回的 `done/status` 只写进副本，调用方看不到。

## 无器件实例

Port 不保存设备地址，每条接口都由调用方传入 `device_address_7bit`，
所以同一条总线挂多个器件时不需要为每个器件做初始化：

```c
core_i2c_mem_read(CORE_I2C_PORT_ENCODER1, 0x36U, reg, CORE_I2C_MEM_ADDR_8BIT,
                   buf, 2U, 10U);
core_i2c_mem_read(CORE_I2C_PORT_ENCODER1, 0x50U, reg, CORE_I2C_MEM_ADDR_8BIT,
                   buf, 2U, 10U);
```

## 移植与使用

> 这一节已并入 **[移植与使用指南](../移植指南.md)**，那里有完整清单：
>
> 1. 复制哪些文件、`port/bare` 与 `port/os` 怎么二选一
> 2. 目录怎么摆（`../../../gpio/...` 是硬编码相对路径）
> 3. 头文件路径、编译清单、C99 要求、三个可覆盖宏
> 4. CubeMX 的 I2C / DMA / NVIC / GPIO 配置逐项
> 5. `i2c_bus_config.h` 槽位表三种写法（硬件 / 软件 / 混用）
> 6. `user_isr_handlers.c` 怎么接、四个回调各管什么
> 7. RTOS 资源表与 `core_i2c_register_mutex/semaphore()`
> 8. 初始化顺序（裸机 / RTOS）
> 9. 三种模式的具体调用代码
> 10. 加总线的三步、软件 I2C 用法、14 条自检表

下面几节只留**接口参考**，不再重复移植步骤。

## Bus 接口

```c
i2c_bus_init();
i2c_bus_is_busy(port);
i2c_bus_is_software(port);
i2c_bus_get_handle(port);          /* 仅硬件槽位 */

/* 逻辑层：逻辑槽位 + 参数校验 + busy 仲裁 */
i2c_bus_write / read / mem_write / mem_read / is_ready(port, ...)
i2c_bus_mem_read_dma / mem_write_dma(port, ...)
i2c_bus_abort_dma(port);
i2c_bus_recover(port);              /* abort 完成中断丢失时强制复位 */

/* direct 层：直接给 HAL 句柄，跳过逻辑槽位和参数校验 */
i2c_bus_write_direct / read_direct(hi2c, ...)
i2c_bus_mem_write_direct / mem_read_direct(hi2c, ...)
i2c_bus_is_ready_direct(hi2c, ...)
i2c_bus_mem_read_dma_direct / mem_write_dma_direct(hi2c, ...)
i2c_bus_abort_dma_direct(hi2c)

i2c_bus_register_rx_callback(port, cb);
i2c_bus_register_tx_callback(port, cb);
i2c_bus_register_error_callback(port, cb);

i2c_bus_irq_mem_rx_complete(hi2c);
i2c_bus_irq_mem_tx_complete(hi2c);
i2c_bus_irq_error(hi2c);
i2c_bus_irq_abort_complete(hi2c);
```

`i2c_bus_mem_read_dma()` 返回 `CORE_I2C_OK` 只代表 DMA 启动成功。

### Bus 内部分两层

只有 `.h` 里声明的才是公开接口，直接层是 `static`，外面看不见。

| 层                                   | 可见性               | 入参              | 做什么                                      |
| ------------------------------------ | -------------------- | ----------------- | ------------------------------------------- |
| 逻辑层 `i2c_bus_mem_read()`        | 公开                 | 逻辑槽位 `port` | 参数校验、硬件/软件分发、`busy` 仲裁      |
| 直接层 `i2c_bus_mem_read_direct()` | **`static`** | HAL 句柄 `hi2c` | 句柄反查槽位、占 `busy`、调 HAL、失败回滚 |

调用关系：

```text
core_i2c_mem_read()            Port
  └─ i2c_bus_mem_read()        Bus 逻辑层（唯一公开面）
       ├─ i2c_bus_mem_read_direct(hi2c, ...)   Bus 直接层（static）
       │    └─ HAL_I2C_Mem_Read(...)
       └─ soft_i2c_mem_read(...)               软件槽位
```

直接层**不吃逻辑槽位号、不做参数校验、不分发软硬件**，只管把这次传输交给 HAL；
但 `busy`、HAL 状态检查、DMA 上下文保存这些必须由它做，因为只有它看得见 HAL。

## 裸机 Port

一条通路：**请求队列 + 时间戳超时**，全部中断驱动，主循环不用 poll。

```c
core_i2c_init();

/* ③ 异步：挂回调，完成时在中断里通知 */
core_i2c_request_t req =
{
    .port                = CORE_I2C_PORT_ENCODER1,
    .device_address_7bit = 0x36U,
    .memory_address      = 0x0CU,
    .address_size        = CORE_I2C_MEM_ADDR_8BIT,
    .p_data              = buf,
    .size                = 2U,
    .is_write            = 0U,
    .timeout_ms          = 10U,
    .pf_done             = on_i2c_done,      /* 可为 NULL，改成轮询 done */
    .p_user_data         = my_ctx,
    .done                = 0U,
};

(void)core_i2c_submit(&req);             /* 入队 + 立刻启动，马上返回 */

/* ① 普通 HAL，CPU 在 HAL 里轮询 */
core_i2c_mem_read(port, dev, mem, addr_size, buf, size, timeout_ms);
core_i2c_mem_write(port, dev, mem, addr_size, data, size, timeout_ms);

/* ② DMA 同步：非阻塞，每次调用立刻返回 */
while (1)
{
    status = core_i2c_mem_read_sync(port, dev, mem, addr_size,
                                    buf, size, timeout_ms);
    if (CORE_I2C_OK == status) { 用(buf); break; }   /* PENDING 就再来一圈 */
}

core_i2c_abort(port);
core_i2c_get_error_code(port);           /* HAL 原始错误码 */
```

### ② 同步怎么工作（裸机）

```text
第 1 次调用 core_i2c_mem_read_sync()
  └─ 端口空闲 → 组装请求 + submit → 记下起始时间 → 立刻 return PENDING

第 2..N 次调用
  ├─ request->done 已置 1  → 取走 status，清 done → return 结果
  ├─ 超时                    → abort → return CORE_I2C_ERR_TIMEOUT
  └─ 否则                    → return PENDING

DMA 完成中断
  └─ 把 status 写进这一笔自己的 request->status，done = 1
```

要点：

1. **函数不阻塞**，CPU 不会停在 `_sync` 里；调用方自己决定多久来查一次。
2. **结果是每笔请求自己的**（`done` + `status`），队列里排多笔不会互相覆盖。
3. **同一器件最多 1 个未完成请求**，重复提交返回 `CORE_I2C_ERR_BUSY`。
4. 队列深度 `CORE_I2C_QUEUE_DEPTH` 默认 4，满了返回 `CORE_I2C_ERR_BUSY`。
5. 超时用 `core_time_get()` 起止时间戳比对，不开硬件定时器。
6. ⚠️ 轮询阶段的入参**会被忽略**，数据进的是**首次提交**时的那个 buffer。

## RTOS Port

判断标准只有一条：**结果在哪里拿**。

| 接口                                        | 搬运     | 结果在哪拿 | 等待对象             |
| ------------------------------------------- | -------- | ---------- | -------------------- |
| `core_i2c_mem_read` / `write`           | 普通 HAL | 返回即完成 | mutex + HAL 内部轮询 |
| `core_i2c_mem_read_sync` / `write_sync` | DMA      | 本函数内阻塞等 | mutex + 完成信号量   |
| `core_i2c_read_async` / `write_async`   | DMA      | 别的地方   | 注册的信号量 / 回调  |

> RTOS 的 `_sync` 仍然是**阻塞**的（`osal_sema_take`），和裸机的非阻塞轮询不同。
> 两套 Port 的 `_sync` 调用形态不一样，别照抄。

```c
/* 资源每总线注册一次 */
(void)core_i2c_init();
(void)core_i2c_register_mutex(CORE_I2C_PORT_ENCODER1,
                              user_mutex_get(USER_MUTEX_IDX_I2C1));
(void)core_i2c_register_semaphore(CORE_I2C_PORT_ENCODER1,
                                  user_sema_get(USER_SEMA_IDX_I2C1_DONE));

/* ① 普通 HAL：锁 + 轮询模式 */
core_i2c_mem_read(port, dev, mem, addr_size, buf, size, timeout_ms);

/* ② DMA，本函数内等完成信号量 */
core_i2c_mem_read_sync(port, dev, mem, addr_size, buf, size, timeout_ms);

/* ③ 异步：DMA 启动即返回，别人在别处等 */
core_i2c_read_async(port, dev, mem, addr_size, buf, size, NULL, NULL);
osal_sema_take(user_sema_get(USER_SEMA_IDX_I2C1_DONE), timeout);
```

要点：

1. `core_i2c_mem_read/write()` 用 **普通 HAL（轮询模式）**，没有 DMA、没有完成回调。
2. `core_i2c_mem_read/write_sync()` 用 **DMA**，在本函数内 `osal_sema_take` **阻塞等**。
3. `core_i2c_read/write_async()` 用 **DMA**，中断里给信号量、可选调回调；`pf_done` 可为 `NULL`。
4. 软件槽位不能用 mutex，Port 自动改成 `osal_critical_enter/exit` + 普通 HAL。
5. 驱动层按数据量选：小数据 `core_i2c_mem_read()`，批量 `_sync()`，高频/不等待 `_async()`。

> 裸机 Port 的 `_sync` 已经改成**非阻塞轮询**（提交后立刻返回 `CORE_I2C_PENDING`），
> RTOS Port 的 `_sync` 仍然是阻塞等信号量。两者调用形态不同。

资源表在 `01_app/app_init/user_task_config/user_task_reso_config.c`，I2C 两项已加好：

```c
{true,  NULL}                 /* USER_MUTEX_IDX_I2C1 */
{true,  true, 1U, 0U, NULL}   /* USER_SEMA_IDX_I2C1_DONE */
```

创建完资源后调一次注册：

```c
status = user_os_resources_init();
...
user_i2c_resource_register();     /* 内部把 mutex/sema 注册给 I2C Port */
```

实现就在同一个文件里：

```c
void user_i2c_resource_register(void)
{
    (void)core_i2c_register_mutex(CORE_I2C_PORT_ENCODER1,
                                  user_mutex_get(USER_MUTEX_IDX_I2C1));
    (void)core_i2c_register_semaphore(CORE_I2C_PORT_ENCODER1,
                                      user_sema_get(USER_SEMA_IDX_I2C1_DONE));
}
```

## 初始化顺序

```text
MX_I2Cx_Init
→ core_i2c_init()                  /* 内部完成 i2c_bus_init */
→ core_i2c_register_mutex/semaphore（仅 RTOS）
→ 启动传输
```

RTOS 还需要先执行 `user_os_resources_init()`，完整顺序见
[移植指南 第 8 节](../移植指南.md#8-初始化顺序)。

## 注意

这些是**使用约定**，移植步骤见[移植指南](../移植指南.md)。

1. 锁归 Port 管，驱动层不要自己再包 mutex；同一总线的并发由 `core_i2c_register_mutex()` 注册的那把锁串行化。
2. 未注册 mutex 时 Port 不加锁，适合单任务场景；未注册 sema 时同步接口退回 Bus 阻塞传输。
3. 软件 I2C 的锁必须是临界区，不能用 mutex；Port 已按槽位类型自动选择。
4. `i2c_bus_mem_read_dma()` 在 ISR 里调用时，HAL 启动阶段仍有地址相位等待；高频 ISR 慎用。
5. 超时后调 `i2c_bus_abort_dma()`，等 `HAL_I2C_AbortCpltCallback()` 清 `busy` 后再重试；
   `CORE_I2C_ABORT_TIMEOUT_MS` 内等不到就调 `i2c_bus_recover()` 强制
   `HAL_I2C_DeInit/Init` 并清 `busy`，避免总线永久卡死。
6. 错误码用 `core_i2c_get_error_code(port)` 读，`HAL_I2C_ERROR_AF` 是从机不应答。
7. 异步回调在中断上下文执行，不要在回调里做阻塞操作。
8. 全工程只保留一份 `HAL_I2C_MemRxCpltCallback` 等 HAL 回调。
9. `HAL_I2C_ErrorCallback` 和 `HAL_I2C_AbortCpltCallback` **不是二选一**：
   前者是传输中真出错，后者只在你调 `Abort_IT` 之后才来；Port 用 `cur_busy`
   只认第一个到的结果。
