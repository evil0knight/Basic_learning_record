# 原生 FreeRTOS 添加任务

[← 返回 添加任务](./添加任务.md) | [← FreeRTOS](./MOC.md) | [← 主页](../../../index.md)

---

适用于已经完成 FreeRTOS 源码移植，但没有 OSAL 和任务配置表的工程。

## 需要修改的文件

| 文件            | 操作                 |
| --------------- | -------------------- |
| 任务模块 `.h` | 声明任务函数         |
| 任务模块 `.c` | 定义任务函数         |
| `main.c`      | 创建任务并启动调度器 |

## 1. 声明任务函数

在任务模块的 `.h` 文件中声明：

```c
void my_task(void *argument);
```

## 2. 定义任务函数

在任务模块的 `.c` 文件中定义：

```c
#include "FreeRTOS.h"
#include "task.h"

void my_task(void *argument)
{
    (void)argument;

    /* 任务初始化 */

    for (;;)
    {
        /* 任务内容 */
        vTaskDelay(pdMS_TO_TICKS(10U));
    }
}
```

任务循环中必须加入延时、队列等待或信号量等待，不能一直占用 CPU。

## 3. 创建任务

在 `main.c` 中包含任务模块头文件：

```c
#include "FreeRTOS.h"
#include "task.h"
#include "my_task.h"
```

完成芯片和外设初始化后创建任务：

```c
TaskHandle_t my_task_handle = NULL;

BaseType_t result = xTaskCreate(
    my_task,
    "MyTask",
    256U,
    NULL,
    tskIDLE_PRIORITY + 1U,
    &my_task_handle);

if (result != pdPASS)
{
    while (1)
    {
    }
}
```

| 参数                      | 内容                                  |
| ------------------------- | ------------------------------------- |
| `my_task`               | 任务函数                              |
| `"MyTask"`              | 任务名称                              |
| `256U`                  | 任务栈深度                            |
| 第一个 `NULL`           | 传给任务函数的参数                    |
| `tskIDLE_PRIORITY + 1U` | 任务优先级                            |
| `&my_task_handle`       | 保存任务句柄，不需要句柄时填 `NULL` |

## 4. 启动调度器

所有初始任务创建完成后调用：

```c
vTaskStartScheduler();

while (1)
{
}
```

`vTaskStartScheduler()` 只调用一次。调度器启动成功后，不会继续执行后面的普通代码。

## 5. 静态 vs 动态内存分配

OS 眼里只有一块连续内存，**没有 .data / .bss / 栈 / 堆 之分**——这些是链接脚本 + 编译器划出来的命名，OS 只认"哪段地址现在归谁用"。

|                | 静态分配                                | 动态分配                                                 |
| -------------- | --------------------------------------- | -------------------------------------------------------- |
| TCB 和栈的位置 | 编译期就定好（占 .bss/.data 数组）      | 运行时才从 Heap 申请                                     |
| 分配时机       | 提前预留                                | 创建任务那一刻                                           |
| 创建函数       | `xTaskCreateStatic()`                 | `xTaskCreate()`                                        |
| 前提           | `configSUPPORT_STATIC_ALLOCATION = 1` | 已加 Heap 实现，`configSUPPORT_DYNAMIC_ALLOCATION = 1` |

静态分配：把 TCB 和栈声明成全局数组，位置在链接时就固定了：

```c
static StackType_t  my_task_stack[256];
static StaticTask_t my_task_tcb;

TaskHandle_t my_task_handle = xTaskCreateStatic(
    my_task,
    "MyTask",
    256U,
    NULL,
    tskIDLE_PRIORITY + 1U,
    my_task_stack,   /* 栈内存，提前占好 */
    &my_task_tcb);   /* TCB 内存，提前占好 */
```

纯静态分配时还需提供空闲任务内存：实现 `vApplicationGetIdleTaskMemory()`，否则调度器起不来。

## main.c 顺序

```text
芯片初始化
→ 外设初始化
→ xTaskCreate() / xTaskCreateStatic() 创建任务
→ vTaskStartScheduler() 启动调度器
```

使用 `xTaskCreate()` 前，工程必须已经加入一个 Heap 实现，并在 `FreeRTOSConfig.h` 中允许动态内存分配；使用 `xTaskCreateStatic()` 则不需要 Heap。
