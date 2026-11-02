# IIC

[← 模块总览](../MOC.md) | [← 主页](../../index.md)

> [IIC知识](./知识/MOC.md)

---

| 内容 | 文件 |
| --- | --- |
| **移植与使用（先看这个）** | **[移植指南](./移植指南.md)** |
| 硬件 I2C Bus 与裸机/RTOS Port | [I2C Bus / Port](./IIC_Port/MOC.md) |
| 裸机 Port | [i2c_port_bare.h](./IIC_Port/port/bare/i2c_port_bare.h) |
| RTOS Port | [i2c_port_os.h](./IIC_Port/port/os/i2c_port_os.h) |
| I2C HAL 中断入口 | [user_isr_handlers.c](./01_app/app_isr_handlers/user_isr_handlers.c) |
| HAL 库硬件 I2C | 由 CubeMX 生成 `i2c.h`、`i2c.c` |
| 知识 | [IIC知识](知识/MOC.md) |

## Port 层

两套实现，二选一复制：

| 目录 | 锁和完成 | 适用 |
| --- | --- | --- |
| `IIC_Port/port/bare/` | `platform_critical_enter/exit` + 标志 | 裸机主循环、FOC 中断 |
| `IIC_Port/port/os/` | 每总线注册的集中 mutex + 集中信号量 | RTOS 任务 |

Port 不保存设备地址，每次调用传入 `dev_addr`，同总线多器件不需要逐个初始化。

裸机（只有一条通路：请求队列）：

- `core_i2c_submit()`：把请求塞进那条总线的队列，入队后就尝试启动。
- 完成和超时都进同一个分发函数，谁先到用谁的结果，主循环不用 poll。
- 超时不开硬件定时器，只在下一次提交时用 `core_time_get()` 瞅一眼。
- `core_i2c_mem_read_sync()` / `core_i2c_mem_write_sync()`：
  `submit + 在本函数内等标志位` 的薄封装。
- 同一器件最多 1 个未完成请求，重复提交返回 `CORE_I2C_ERR_BUSY`。
- 先 `core_i2c_init()`，再提交请求。

RTOS：

- `core_i2c_register_mutex(port, mutex)` / `core_i2c_register_semaphore(port, sema)`：
  每总线注册一次，驱动层不碰锁。
- `core_i2c_mem_read_sync()`：加锁 + 启动 DMA + 等信号量，锁覆盖整个传输，
  超时自动 abort。
- `core_i2c_read_async()`：启动后返回，完成时给信号量并在中断上下文回调。
- 不要在中断里调 `_sync`。

## 移植与使用

完整步骤、文件清单、目录约束、工程配置、槽位表、中断接入、三种模式怎么调、
自检表，全部在 **[移植与使用指南](./移植指南.md)**。

一句话版本：

1. 复制 `IIC_Port/bus/inc`、`IIC_Port/bus/src` 全部文件。
2. 按环境复制 `IIC_Port/port/bare` **或** `IIC_Port/port/os` 一套（不能都要）。
3. 加 `bus/inc`、所选 Port 目录、`gpio/inc`（裸机再加 `time/inc`、`critical/inc`）。
4. 编译 `i2c_bus.c`、`soft_i2c.c`、所选 Port、`gpio_port.c`。
5. 改 `i2c_bus_config.h` 填槽位；拷 `user_isr_handlers.c`；RTOS 再配资源表。

目录层级和 include 路径是硬编码的，**动位置前先看指南第 2 节**。
