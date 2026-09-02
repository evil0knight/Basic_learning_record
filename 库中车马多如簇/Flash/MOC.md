# Flash

[← 模块总览](../MOC.md) | [裸机使用 Driver](./裸机使用Driver.md)

> [Flash知识](./知识/MOC.md)

外部 Flash（W25Q64）驱动栈：事件驱动 + 单任务串行，天然互斥（无需 mutex）。

## 文件结构

| 目录                                                                                 | 位置（目标工程）                           |
| ------------------------------------------------------------------------------------ | ------------------------------------------ |
| [bsp_wrapper_flash](./bsp_wrapper_flash/bsp_wrapper_flash.h)                            | `02_Platform/platform_bsp/flash/`        |
| [bsp_adapter_port_flash](./bsp_adapter_port_flash/bsp_adapter_port_flash.h)             | `02_Platform/platform_bsp/flash/`（OS / 裸机二选一，`FLASH_BARE_METAL` 切换） |
| [W25Q](./W25Q/MOC.md)                                                                   | `03_Bsp/bsp_storage/`                    |
| [service_externflash_manage](./service_externflash_manage/service_externflash_manage.h) | `01_Service/service_externflash_manage/` |
| [STM32F411](./STM32F411/MOC.md)                                                         | `03_Bsp/bsp_storage/`                    |

## 接入目标工程

### 1. 申请资源

**① 两个任务**

[user_task_reso_config.h](../代码架构/firmware/01_app/app_init/user_task_config/user_task_reso_config.h)(填写步骤见 [app_init 笔记](../代码架构/firmware/01_app/app_init/MOC.md))的 `user_task_idx_t` 枚举加索引：

```c
typedef enum
{
    /* 已有索引... */
    USER_TASK_IDX_EXT_FLASH_DRV,    /* flash_handler_thread */
    USER_TASK_IDX_EXT_FLASH_TASK,   /* storage_manager_task */
    USER_TASK_IDX_MAX
} user_task_idx_t;
```

[user_task_reso_config.c](../代码架构/firmware/01_app/app_init/user_task_config/user_task_reso_config.c)(填写步骤见 [app_init 笔记](../代码架构/firmware/01_app/app_init/MOC.md)) 的 `g_user_tasks[]` 配置表填（顺序与枚举一致）：

```c
user_task_config_t g_user_tasks[USER_TASK_IDX_MAX] =
{
    /* 启用, 名称, 入口, 栈大小, 优先级, 参数, 句柄 */
    {true, "ExtFlashDrv",  flash_handler_thread, 1024U, 3U, &flash_handler_all_input_arg, NULL},
    {true, "ExtFlashTask", storage_manager_task, 1024U, 2U, NULL, NULL}
};
```

> `flash_handler_all_input_arg` 由 [bsp_adapter_port_flash](./bsp_adapter_port_flash/bsp_adapter_port_flash.c) 定义，作为 Handler 线程的 argument。

**② 一个信号量**（外部 Flash 完成信号量）

[user_task_reso_config.h](../代码架构/firmware/01_app/app_init/user_task_config/user_task_reso_config.h)(填写步骤见 [app_init 笔记](../代码架构/firmware/01_app/app_init/MOC.md)) 的 `user_sema_idx_t` 枚举加索引：

```c
typedef enum
{
    /* 已有索引... */
    USER_SEMA_IDX_EXT_FLASH,
    USER_SEMA_IDX_MAX
} user_sema_idx_t;
```

[user_task_reso_config.c](../代码架构/firmware/01_app/app_init/user_task_config/user_task_reso_config.c)(填写步骤见 [app_init 笔记](../代码架构/firmware/01_app/app_init/MOC.md))的 `g_user_semaphores[]` 配置表填：

```c
user_sema_config_t g_user_semaphores[USER_SEMA_IDX_MAX] =
{
    /* 启用, 是否二值, 最大计数, 初始计数, 句柄 */
    {true, true, 1U, 0U, NULL}
};
```

> 事件组由 `storage_manager_task` 内部创建，不需要资源表申请。

代码里按索引取句柄：`user_sema_get(USER_SEMA_IDX_EXT_FLASH)`。

### 2. 配置宏

在 [bsp_adapter_port_flash.h](./bsp_adapter_port_flash/bsp_adapter_port_flash.h) 配置：

 `FLASH_SPI_PORT`Flash 用的 SPI 槽位号，即 `spi_port_config.h` 的 `g_spi_slots[]` 数组下标（0~7）

`FLASH_SPI_TIMEOUT_MS`单次 SPI 传输超时（毫秒）

### 3. 注册流程

1. 先完成 [SPI Port](../SPI/MOC.md) 移植，`spi_port_config.h` 里为 Flash 配置一个 SPI slot；
2. 在目标工程 `app_periph_register()` 里调用：

```c
#include "bsp_adapter_port_flash.h"

void app_periph_register(void)
{
    /* 其他外设注册... */
    bsp_adapter_flash_register();
}
```

3. 上层通过异步读写服务访问（`addr` 为绝对地址，分区由业务层自己定义）：

```c
#include "service_externflash_manage.h"

extern_flash_write(addr, data, size);   /* 写，内部先擦后写 */
extern_flash_read(addr, size, buf);     /* 读 */
extern_flash_erase(addr, size);         /* 擦除扇区 */
```

### 4. 可复用与解耦

| 组件                                          | 是否 flash 专用               |
| --------------------------------------------- | ----------------------------- |
| `spi_w25q64_interface_t`（SPI 逐字节接口）  | 通用，其他 SPI 器件可照此模式 |
| 事件驱动 Handler 模式（事件队列 + 单任务）    | 通用，其他 BSP 外设可照此模式 |
| 函数表注册（`externflash_drv_t` + `reg`） | 通用，其他 Wrapper 可照此模式 |
| W25Q64 寄存器操作 / 命令码                    | flash 专用                    |

## 依赖

- OSAL（队列 / 信号量 / 事件组 / 任务 / 延时 / 临界区）
- SPI Port（core_spi 逐字节接口）—— 见 [SPI](../SPI/MOC.md)
