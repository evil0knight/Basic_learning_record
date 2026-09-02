# SPI Port

[← SPI模块](../MOC.md) | [← 主页](../../../index.md)

> 先由CubeMX生成硬件SPI依赖，配置方法见[SPI模块](../MOC.md)。

---

| 文件                                                                   | 作用                                   |
| ---------------------------------------------------------------------- | -------------------------------------- |
| [spi_port.h](./inc/spi_port.h) / [spi_port.c](./src/spi_port.c)              | 统一逻辑槽位、HAL传输、CS、锁和事务API |
| [spi_port_config.h](./inc/spi_port_config.h)                              | 硬件/软件SPI槽位和板级资源配置         |
| [soft_spi.h](./src/soft_spi.h) / [soft_spi.c](./src/soft_spi.c)              | 软件SPI内部接口和字节时序              |
| [GPIO Port](../../代码架构/firmware/04_platform/platform_mcu/gpio/MOC.md) | 提供CS和软件SPI所需的直接GPIO接口      |

## 移植文件

1. 复制 `SPI_Port/inc`和 `SPI_Port/src`内全部文件。
2. 添加 `SPI_Port/inc`和 `SPI_Port/src`头文件搜索路径。
3. 将 `spi_port.c`、`soft_spi.c`加入工程编译。
4. 确认工程已有CubeMX生成的 `main.h`、`spi.h`、`spi.c`和HAL SPI Driver。
5. `spi_port.c`依赖 `gpio_port.h`的直接GPIO接口，同时复制[GPIO Port](../../代码架构/firmware/04_platform/platform_mcu/gpio/MOC.md)或替换对应调用。

## 硬件SPI配置

在 `spi_port_config.h`包含目标工程的 `spi.h`，再填写槽位初始化器：

```c
#include "spi.h"

static st_core_spi_slot_t g_spi_slots[CORE_SPI_PORT_MAX] = {
    [0U] = {
        .type = CORE_SPI_TYPE_HARDWARE,
        .config.hardware = {
            .handle = &hspi_x,
            .cs_port = DEVICE_CS_GPIO_Port,
            .cs_pin = DEVICE_CS_Pin,
            .cs_active_state = GPIO_PIN_RESET,
            .lock = NULL,
            .unlock = NULL
        }
    }
};
```

将 `hspi_x`、`DEVICE_CS_GPIO_Port`和 `DEVICE_CS_Pin`替换为目标工程中 `spi.h`和 `main.h`已有的符号。数组下标就是设备Driver使用的SPI槽位。

## 软件SPI配置

软件SPI引脚在CubeMX中配置为普通GPIO。在 `spi_port_config.h`声明目标工程的延时函数，再填写槽位初始化器：

```c
extern void board_delay_us(uint32_t delay_us);

static st_core_spi_slot_t g_spi_slots[CORE_SPI_PORT_MAX] = {
    [0U] = {
        .type = CORE_SPI_TYPE_SOFTWARE,
        .config.software = {
            .sck_port = DEVICE_SCK_GPIO_Port,
            .sck_pin = DEVICE_SCK_Pin,
            .mosi_port = DEVICE_MOSI_GPIO_Port,
            .mosi_pin = DEVICE_MOSI_Pin,
            .miso_port = DEVICE_MISO_GPIO_Port,
            .miso_pin = DEVICE_MISO_Pin,
            .cs_port = DEVICE_CS_GPIO_Port,
            .cs_pin = DEVICE_CS_Pin,
            .cs_active_state = GPIO_PIN_RESET,
            .mode = CORE_SPI_MODE_0,
            .bit_order = CORE_SPI_MSB_FIRST,
            .half_period_us = 2U,
            .delay_us = board_delay_us,
            .lock = NULL,
            .unlock = NULL
        }
    }
};
```

`board_delay_us()`由目标工程提供。只使用软件SPI时无需调用 `MX_SPIx_Init()`。

## 多总线与多设备

每个数组下标对应一个设备连接。不同硬件总线填写不同 `hspi`；同一总线的多个设备填写相同 `hspi`和不同CS。设备Driver初始化时传入对应数组下标。

## 锁策略（先看这个）

| 总线类型 | OS 环境 | lock / unlock 填什么 |
| --- | --- | --- |
| 硬件 SPI | OS | 互斥量（mutex），防多任务并发 |
| 硬件 SPI | 裸机 | NULL（单线程，无需锁） |
| 软件 SPI | OS | 临界区（关中断），防中断打断时序 |
| 软件 SPI | 裸机 | 关中断（__disable_irq / __enable_irq） |

软件 SPI 靠 GPIO 逐位翻转时序，中途被中断打断会乱，所以软件槽位**必须**关中断保护；硬件 SPI 由外设自己管理时序，只需多任务时用互斥量串行化。

## 裸机移植

1. 硬件槽位锁填 NULL；软件槽位填关中断：

```c
/* 软件 SPI：关中断保护 GPIO 时序 */
static int soft_spi_lock(uint32_t timeout_ms)
{
    (void)timeout_ms;
    __disable_irq();
    return 0;
}

static void soft_spi_unlock(void)
{
    __enable_irq();
}
```

软件槽位填 `.lock = soft_spi_lock`、`.unlock = soft_spi_unlock`；硬件槽位填 `.lock = NULL`、`.unlock = NULL`。

2. 硬件 SPI 初始化顺序：

```text
MX_GPIO_Init
→ MX_SPIx_Init
→ Driver或BSP Adapter初始化
```

3. 软件 SPI 只执行 `MX_GPIO_Init`，随后初始化 Driver 或 BSP Adapter。

## OS 移植

1. 硬件 SPI：为每条共享硬件总线申请一个互斥量，实现锁回调：

```c
#include "osal.h"
#include "user_task_reso_config.h"

static int hw_spi_lock(uint32_t timeout_ms)
{
    osal_mutex_handle_t mutex = user_mutex_get(USER_MUTEX_IDX_SPI2);
    (void)timeout_ms;

    if (mutex == NULL)
    {
        return -1;
    }

    return (osal_mutex_take(mutex, OSAL_WAIT_FOREVER) == OSAL_SUCCESS) ? 0 : -1;
}

static void hw_spi_unlock(void)
{
    osal_mutex_handle_t mutex = user_mutex_get(USER_MUTEX_IDX_SPI2);

    if (mutex != NULL)
    {
        (void)osal_mutex_give(mutex);
    }
}
```

硬件槽位填 `.lock = hw_spi_lock`、`.unlock = hw_spi_unlock`。

2. 软件 SPI：用临界区保护 GPIO 时序：

```c
#include "osal.h"

static int sw_spi_lock(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return (osal_critical_enter() == OSAL_SUCCESS) ? 0 : -1;
}

static void sw_spi_unlock(void)
{
    (void)osal_critical_exit();
}
```

软件槽位填 `.lock = sw_spi_lock`、`.unlock = sw_spi_unlock`。

3. 初始化顺序：

```text
MX_GPIO_Init
→ MX_SPIx_Init
→ user_os_resources_init（创建互斥量）
→ Driver或BSP Adapter初始化
→ 创建业务任务
→ 启动调度器
```

每条硬件 SPI 总线分别创建互斥量和锁回调；同一总线的多个 CS 设备共用该互斥量。

## Driver调用

器件Driver只调用 `core_spi_begin()`、`core_spi_write()`、`core_spi_read()`、`core_spi_transfer()`和 `core_spi_end()`，返回值统一使用 `en_core_spi_status_t`。
