# I2C Port

[← IIC模块](../MOC.md) | [← 主页](../../../index.md)

> 先看[IIC模块](../MOC.md),然后再来配置接口层

---

| 文件                                                                   | 作用                                       |
| ---------------------------------------------------------------------- | ------------------------------------------ |
| [i2c_port.h](./inc/i2c_port.h) / [i2c_port.c](./src/i2c_port.c)              | 统一状态、静态总线表、HAL分发和阻塞事务API |
| [i2c_port_config.h](./inc/i2c_port_config.h)                              | 硬件/软件I2C槽位和板级资源配置             |
| [soft_i2c.h](./src/soft_i2c.h) / [soft_i2c.c](./src/soft_i2c.c)              | 软件I2C内部接口和START、STOP、ACK/NACK时序 |
| [GPIO Port](../../代码架构/firmware/04_platform/platform_mcu/gpio/MOC.md) | 软件I2C所需的直接GPIO接口                  |

## 移植文件

1. 复制 `IIC_Port/inc/i2c_port.h`和 `IIC_Port/src`内全部文件。
2. 添加 `IIC_Port/inc`、`IIC_Port/src`和GPIO Port头文件路径。
3. 将 `i2c_port.c`、`soft_i2c.c`和 `gpio_port.c`加入工程编译。
4. 确认工程已有CubeMX生成的 `main.h`；硬件I2C还需要 `i2c.h`、`i2c.c`和HAL I2C Driver。

## 硬件I2C配置

在 `i2c_port_config.h`包含目标工程的 `i2c.h`，再填写槽位初始化器：

```c
#include "i2c.h"

static st_core_i2c_slot_t g_i2c_slots[CORE_I2C_PORT_MAX] = {
    [0U] = {
        .type = CORE_I2C_TYPE_HARDWARE,
        .config.hardware = {
            .handle = &hi2c1,
            .lock = NULL,
            .unlock = NULL
        }
    }
};
```

## 软件I2C配置

1. 在CubeMX中将SCL、SDA配置为开漏输出并确认总线有上拉。
2. 将GPIO的User Label分别填写为 `SENSOR_SCL`、`SENSOR_SDA`，CubeMX会在 `main.h`中生成：

```c
#define SENSOR_SCL_Pin GPIO_PIN_6
#define SENSOR_SCL_GPIO_Port GPIOB
#define SENSOR_SDA_Pin GPIO_PIN_7
#define SENSOR_SDA_GPIO_Port GPIOB
```

引脚和端口以实际硬件为准；不使用User Label时，直接把下方宏替换为 `GPIOB`、`GPIO_PIN_6`等实际参数。

3. 在 `i2c_port_config.h`填写槽位初始化器：

```c
extern void board_delay_us(uint32_t delay_us);

static st_core_i2c_slot_t g_i2c_slots[CORE_I2C_PORT_MAX] = {
    [0U] = {
        .type = CORE_I2C_TYPE_SOFTWARE,
        .config.software = {
            .scl_port = SENSOR_SCL_GPIO_Port,
            .scl_pin = SENSOR_SCL_Pin,
            .sda_port = SENSOR_SDA_GPIO_Port,
            .sda_pin = SENSOR_SDA_Pin,
            .half_period_us = 5U,
            .delay_us = board_delay_us,
            .lock = NULL,
            .unlock = NULL
        }
    }
};
```

软件I2C不需要调用 `MX_I2Cx_Init()`。

## 锁策略（先看这个）

| 总线类型 | OS 环境 | lock / unlock 填什么 |
| --- | --- | --- |
| 硬件 I2C | OS | 互斥量（mutex），防多任务并发 |
| 硬件 I2C | 裸机 | NULL（单线程，无需锁） |
| 软件 I2C | OS | 临界区（关中断），防中断打断时序 |
| 软件 I2C | 裸机 | 关中断（__disable_irq / __enable_irq） |

软件 I2C 靠 GPIO 逐位翻转时序，中途被中断打断会乱，所以软件槽位**必须**关中断保护；硬件 I2C 由外设自己管理时序，只需多任务时用互斥量串行化。

## 裸机移植

1. 硬件槽位锁填 NULL；软件槽位填关中断：

```c
/* 软件 I2C：关中断保护 GPIO 时序 */
static int soft_i2c_lock(uint32_t timeout_ms)
{
    (void)timeout_ms;
    __disable_irq();
    return 0;
}

static void soft_i2c_unlock(void)
{
    __enable_irq();
}
```

软件槽位填 `.lock = soft_i2c_lock`、`.unlock = soft_i2c_unlock`；硬件槽位填 `.lock = NULL`、`.unlock = NULL`。

2. 初始化顺序：`MX_GPIO_Init → MX_I2Cx_Init → Driver或BSP Adapter初始化`。
3. 软件 I2C 省略 `MX_I2Cx_Init`。
4. 主循环与中断不得同时访问同一条总线（软件 I2C 靠上面的关中断保证）。

## OS 移植

1. 硬件 I2C：为每条共享硬件总线申请一个互斥量，实现锁回调：

```c
#include "osal.h"
#include "user_task_reso_config.h"

static int hw_i2c_lock(uint32_t timeout_ms)
{
    osal_mutex_handle_t mutex = user_mutex_get(USER_MUTEX_IDX_I2C1);
    (void)timeout_ms;

    if (mutex == NULL)
    {
        return -1;
    }

    return (osal_mutex_take(mutex, OSAL_WAIT_FOREVER) == OSAL_SUCCESS) ? 0 : -1;
}

static void hw_i2c_unlock(void)
{
    osal_mutex_handle_t mutex = user_mutex_get(USER_MUTEX_IDX_I2C1);

    if (mutex != NULL)
    {
        (void)osal_mutex_give(mutex);
    }
}
```

硬件槽位填 `.lock = hw_i2c_lock`、`.unlock = hw_i2c_unlock`。Port 在完整事务期间持锁，NACK、超时和其他错误路径同样释放锁。

2. 软件 I2C：用临界区保护 GPIO 时序：

```c
#include "osal.h"

static int sw_i2c_lock(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return (osal_critical_enter() == OSAL_SUCCESS) ? 0 : -1;
}

static void sw_i2c_unlock(void)
{
    (void)osal_critical_exit();
}
```

软件槽位填 `.lock = sw_i2c_lock`、`.unlock = sw_i2c_unlock`。

3. 初始化顺序：`MX_GPIO_Init → MX_I2Cx_Init → OS资源初始化（创建互斥量）→ Driver或BSP Adapter初始化 → 创建任务 → 启动调度器`。

## 调用接口

外设Driver调用 `core_i2c_write()`、`core_i2c_read()`、`core_i2c_mem_write()`、`core_i2c_mem_read()`和 `core_i2c_is_ready()`。
