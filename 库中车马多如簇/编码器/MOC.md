# 编码器

[← MOC](../MOC.md) | [← 主页](../../index.md)

---

## 资源在哪里注册

AS5600 使用 I2C，7 位地址为 `0x36`，寄存器地址宽度为 8 位。硬件需要供电、共地、带上拉的 SDA/SCL 及配套磁铁。

下表路径以 `03_Firmware/` 为根目录。

| 位置                                                                                               | 配置内容                                                                                                                        |
| -------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| `04_platform/platform_mcu/i2c/bus/inc/i2c_bus_config.h`                                          | 将逻辑槽位 `CORE_I2C_PORT_ENCODER1` 绑定到 I2C 句柄，如 `&hi2c1`                                                            |
| [bsp_adapter_encoder.c](03_Firmware/04_platform/platform_bsp/encoder/adapter/bsp_adapter_encoder.c) | 在 `g_a_bsp_adapter_encoder_iic` 中绑定同步读写、异步提交、逻辑槽位和超时；毫秒延时由 `bsp_adapter_encoder_delay_ms()` 提供 |
| [bsp_adapter_encoder.h](03_Firmware/04_platform/platform_bsp/encoder/adapter/bsp_adapter_encoder.h) | 在 `bsp_encoder_id_t` 中定义设备编号，与 Adapter 配置表下标对应                                                               |
| [bsp_wrapper_encoder.h](03_Firmware/04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h) | `BSP_WRAPPER_ENCODER_DRV_MAX_NUM` 决定最多注册的设备数，默认 2 台                                                             |
| 板级初始化入口                                                                                     | 平台资源初始化后，调用一次 `bsp_adapter_encoder_register()` 注册设备操作表                                                    |

Adapter 配置项：

```c
[BSP_ENCODER_ID_MOTOR] =
{
    .pf_mem_read = core_i2c_mem_read,
    .pf_mem_write = core_i2c_mem_write,
    .pf_submit = core_i2c_submit,
    .port = CORE_I2C_PORT_ENCODER1,
    .u32_timeout_ms = BSP_ADAPTER_ENCODER_I2C_TIMEOUT_MS
}
```

同步读写返回成功时必须已完成传输；异步提交完成后通过请求中的 `pf_done` 通知 Driver。默认超时 10 ms，`core_time_get()` 提供持续递增的毫秒时基。

多颗 AS5600 地址相同，需要独立总线或通道切换，并为每个设备补齐编号和配置。

工程依赖如下，目录层级需要与相对头文件路径一致：

| 资源                | 位置及要求                                                                                                                     |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| I2C Bus 与裸机 Port | `04_platform/platform_mcu/i2c/bus/`、`04_platform/platform_mcu/i2c/port/bare/`，提供类型、初始化、读写、异步提交及事件分发 |
| 时间基准            | `04_platform/platform_mcu/time/`，提供 `core_time_get()`                                                                   |
| 临界区              | `04_platform/platform_mcu/critical/`，提供 `platform_critical_enter/exit()`，保存并恢复中断状态                            |
| AB 双缓冲           | `06_middlewares/buffer/ab_buffer.h/.c`，保存和消费 DMA 样本                                                                  |
| 应用状态类型        | `01_app/app_control/app_control.h`，提供 Service 使用的 `app_status_t` 和 `APP_STATUS_*`                                 |
| HAL 外设资源        | 初始化 GPIO、I2C、DMA，启用相应 NVIC，并在中断处理函数中调用 HAL                                                               |

DMA 的 HAL 回调需要转发到 Bus；已有同名回调时合并到现有函数。

| HAL 回调                              | 转发接口                                |
| ------------------------------------- | --------------------------------------- |
| `HAL_I2C_MemRxCpltCallback(handle)` | `i2c_bus_irq_mem_rx_complete(handle)` |
| `HAL_I2C_MemTxCpltCallback(handle)` | `i2c_bus_irq_mem_tx_complete(handle)` |
| `HAL_I2C_ErrorCallback(handle)`     | `i2c_bus_irq_error(handle)`           |
| `HAL_I2C_AbortCpltCallback(handle)` | `i2c_bus_irq_abort_complete(handle)`  |

## 如何使用

### 初始化与采样

将 `service_foc.c`、`bsp_wrapper_encoder.c`、`bsp_adapter_encoder.c`、`bsp_encoder_handler.c`、`bsp_encoder_driver.c` 及平台依赖加入编译。

```text
初始化时钟、GPIO、DMA、I2C、毫秒时基
→ core_i2c_init()
→ bsp_adapter_encoder_register()
→ service_foc_init()
→ 启用周期采样节拍
```

检查 `core_i2c_init()` 返回 `CORE_I2C_OK`、`service_foc_init()` 返回 `APP_STATUS_OK` 后，再启用采样节拍。注册只执行一次，采样期间不要重复初始化 Driver。

[Service](03_Firmware/02_service/service_foc/service_foc.c) 的 `service_foc_init()` 会挂载编码器并发起首笔 DMA。在 **20 kHz** 定时入口调用 `service_foc_encoder_tick()`，内部每 4 次提交一笔采样，即 **5 kHz**；每个节拍消费已完成数据并更新缓存。

采样频率为“入口频率 ÷ `SERVICE_FOC_ENCODER_DIV`”，需结合 I2C 速率和总线负载设置。DMA 完成回调不会自动续传。

主循环可调用 `service_foc_poll(now_ms)`；它仅在尚未初始化时尝试初始化，不触发采样。初始化返回值必须单独检查，后续 `poll()` 成功不能证明首笔 DMA 成功。

### 读取角度

接口声明见 [service_foc.h](03_Firmware/02_service/service_foc/service_foc.h)。以下为业务函数内的调用片段，头文件搜索路径以 `03_Firmware/` 为根目录。

```c
#include "02_service/service_foc/service_foc.h"

uint16_t angle_count = 0U;
if (APP_STATUS_OK == service_foc_get_angle_count(&angle_count))
{
    float mech_rad = service_foc_count_to_mech_rad(angle_count);
    (void)mech_rad;
}
```

计数范围为 `0~4095`，换算函数按整圈 4096 计数计算机械角弧度。缓存来自 ANGLE 寄存器，修改芯片测量范围后，业务换算也需调整。

读取缓存不会判断数据是否新鲜：首个样本前为 0，采样失败时保持旧值。使用 Service 时，由它统一消费 DMA 双缓冲，业务读取缓存。

### 直接读取与配置

不使用 Service 时，注册 Adapter 后调用 `bsp_wrapper_encoder_init(id)`，再通过 [Wrapper](03_Firmware/04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h) 操作设备；`id` 为 `BSP_ENCODER_ID_MOTOR` 等设备编号。

| 接口                                               | 用法                                                                            |
| -------------------------------------------------- | ------------------------------------------------------------------------------- |
| `bsp_wrapper_encoder_read_raw_angle(id, &count)` | 阻塞读取原始角度，度数为 `count × 360 / 4096`                                |
| `bsp_wrapper_encoder_read_angle(id, &count)`     | 阻塞读取经过范围缩放和滤波的 ANGLE                                              |
| `bsp_wrapper_encoder_start_dma(id)`              | 提交一笔异步采样，需按周期反复调用；上一笔未结束返回 `ERR_BUSY`               |
| `bsp_wrapper_encoder_get_dma_angle(id, &count)`  | 消费完成样本，返回 `BSP_WRAPPER_ENCODER_OK` 时使用结果                        |
| `bsp_wrapper_encoder_stop_dma(id)`               | 清除采样使能，允许在飞请求完成；调用前先停用外部周期提交入口                    |
| `bsp_wrapper_encoder_set_range(id, &range)`      | 设置 `u16_start/u16_stop`，范围 `0~4095`；相等为整圈，否则跨度至少 205 计数 |
| `bsp_wrapper_encoder_set_power_mode(id, &power)` | 设置正常或低功耗模式，以及低功耗看门狗                                          |
| `bsp_wrapper_encoder_set_filter(id, &filter)`    | 设置慢滤波、快滤波阈值及迟滞                                                    |

参数结构见 [Handler 头文件](03_Firmware/05_bsp/bsp_encoder/handler/bsp_encoder_handler.h)，枚举见 [Driver 头文件](03_Firmware/05_bsp/bsp_encoder/driver/bsp_encoder_driver.h)。配置时先停用周期提交并等待在飞请求完成，串行访问设备和总线。

范围、电源和滤波写入易失寄存器。永久保存使用 `bsp_wrapper_encoder_save_config()`：OTP 的 ANGLE 最多烧录三次，SETTING 仅一次；调用前核对芯片手册规定的供电和烧录条件，失败后不能自动重试。
