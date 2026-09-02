# app_init

[← 代码架构库](../../../MOC.md) | [← 嵌入式工程架构](../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../MOC.md) | [← 主页](../../../../../index.md)

---

应用初始化入口，负责注册平台接口、创建公共 OS 资源并启动业务任务。

| 内容                                                                                                                                                                                   | 作用                                     |
| -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------- |
| [user_init.h](./user_init.h) / [user_init.c](./user_init.c) | 声明并实现应用统一初始化入口 |
| [user_periph_setup.h](./platform_io_register/user_periph_setup.h) / [user_periph_setup.c](./platform_io_register/user_periph_setup.c) | 集中调用各 Adapter 的注册函数 |
| [user_task_reso_config.h](./user_task_config/user_task_reso_config.h) / [user_task_reso_config.c](./user_task_config/user_task_reso_config.c) | 保存任务、队列、互斥量、信号量等资源配置 |

## user_init配置:

1. 在工程启动入口包含 `user_init.h`。
2. OSAL 初始化完成后调用：

```c
if (user_app_init() != OSAL_SUCCESS)
{
    /* 初始化失败处理 */
}
```

## user_periph_setup配置:

1. 在 `user_periph_setup.c` 包含 Adapter 头文件。
2. 在 `app_periph_register()` 中添加注册函数：

```c
#include "bsp_adapter_xxx.h"

void app_periph_register(void)
{
    bsp_adapter_xxx_register();
}
```

## user_task_reso_config配置:

下面以一个 `xxx` 模块需要一个任务、一个队列、一个互斥量和一个二值信号量为例。

1. 在模块头文件中声明任务函数和队列消息类型：

```c
#include <stdint.h>

typedef struct
{
    uint32_t value;
} xxx_message_t;

void xxx_task(void *argument);
```

2. 在 `rtos_config.h` 添加资源参数：

```c
#define CFG_TASK_XXX_STACK_SIZE 256U
#define CFG_TASK_XXX_PRIORITY   2U
#define CFG_QUEUE_XXX_DEPTH     8U
```

3. 在 `user_task_reso_config.h` 添加索引，`IDX_MAX` 必须放在最后：

```c
typedef enum
{
    USER_TASK_IDX_XXX = 0,
    USER_TASK_IDX_MAX
} user_task_idx_t;

typedef enum
{
    USER_QUEUE_IDX_XXX = 0,
    USER_QUEUE_IDX_MAX
} user_queue_idx_t;

typedef enum
{
    USER_MUTEX_IDX_XXX = 0,
    USER_MUTEX_IDX_MAX
} user_mutex_idx_t;

typedef enum
{
    USER_SEMA_IDX_XXX = 0,
    USER_SEMA_IDX_MAX
} user_sema_idx_t;
```

多个资源继续向下添加，配置表顺序必须与枚举顺序一致：

```c
typedef enum
{
    USER_TASK_IDX_XXX = 0,
    USER_TASK_IDX_YYY,
    USER_TASK_IDX_MAX
} user_task_idx_t;
```

4. 在 `user_task_reso_config.c` 包含模块头文件：

```c
#include "xxx.h"
```

5. 填写四张配置表：

```c
user_task_config_t g_user_tasks[USER_TASK_IDX_MAX] =
{
    /* 启用, 名称, 入口, 栈大小, 优先级, 参数, 句柄 */
    {true, "xxx", xxx_task, CFG_TASK_XXX_STACK_SIZE,
     CFG_TASK_XXX_PRIORITY, NULL, NULL}
};

user_queue_config_t g_user_queues[USER_QUEUE_IDX_MAX] =
{
    /* 启用, 深度, 单条消息大小, 句柄 */
    {true, CFG_QUEUE_XXX_DEPTH, sizeof(xxx_message_t), NULL}
};

user_mutex_config_t g_user_mutexes[USER_MUTEX_IDX_MAX] =
{
    /* 启用, 句柄 */
    {true, NULL}
};

user_sema_config_t g_user_semaphores[USER_SEMA_IDX_MAX] =
{
    /* 启用, 是否二值, 最大计数, 初始计数, 句柄 */
    {true, true, 1U, 0U, NULL}
};
```

- 第一个 `true`：创建这个资源；改成 `false` 就会跳过。
- 句柄初始值统一写 `NULL`，创建成功后由初始化函数自动保存。
- 二值信号量第二项写 `true`。
- 计数信号量第二项写 `false`，再填写最大计数和初始计数。

6. 其他模块通过枚举索引获取句柄：

```c
osal_task_handle_t task = user_task_get(USER_TASK_IDX_XXX);
osal_queue_handle_t queue = user_queue_get(USER_QUEUE_IDX_XXX);
osal_mutex_handle_t mutex = user_mutex_get(USER_MUTEX_IDX_XXX);
osal_sema_handle_t semaphore = user_sema_get(USER_SEMA_IDX_XXX);
```

只需要哪种资源就配置哪张表，其余占位项保持 `false`。

---

```
工程启动入口
  → user_app_init()
  → user_os_resources_init()
     → 遍历 g_user_queues
     → 遍历 g_user_mutexes
     → 遍历 g_user_semaphores
  → user_tasks_start()
     → 遍历 g_user_tasks
```

工程启动入口一般是 `Core/Src/main.c` 中的 `main()`，在 MCU、时钟、外设初始化完成后，启动调度器前调用：

```c
#include "user_init.h"

if (user_app_init() != OSAL_SUCCESS)
{
    while (1)
    {
    }
}

osal_task_start_scheduler();//这些表都在里面遍历调用
```
