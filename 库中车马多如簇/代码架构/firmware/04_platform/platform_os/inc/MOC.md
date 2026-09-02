# OSAL inc

[← 代码架构库](../../../../MOC.md) | [← 嵌入式工程架构](../../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../../MOC.md) | [← 主页](../../../../../../index.md)

---

对 App、Service 公开统一 OS 接口，上层只包含这些头文件，不使用 FreeRTOS 类型。

| 文件 | 作用 |
| --- | --- |
| [osal.h](./osal.h) | 统一包含入口 |
| [osal_types.h](./osal_types.h) | 公共类型 |
| [osal_config.h](./osal_config.h) | OS选择和公共配置 |
| [osal_error.h](./osal_error.h) | 统一错误码 |
| [osal_task.h](./osal_task.h) | 任务接口 |
| [osal_queue.h](./osal_queue.h) | 队列接口 |
| [osal_mutex.h](./osal_mutex.h) | 互斥量接口 |
| [osal_sema.h](./osal_sema.h) | 信号量接口 |
| [osal_timer.h](./osal_timer.h) | 软件定时器接口 |
| [osal_heap.h](./osal_heap.h) | 堆内存接口 |
