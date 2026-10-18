# OSAL os_wrapper

[← 代码架构库](../../../../MOC.md) | [← 嵌入式工程架构](../../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../../MOC.md) | [← 主页](../../../../../../index.md)

---

负责参数检查、统一类型和错误码，再调用 os_implementation，不直接调用业务代码。

| 资源 | 内部声明 | Wrapper实现 |
| --- | --- | --- |
| 任务、任务通知 | [osal_internal_task.h](./inc/osal_internal_task.h) | [osal_task.c](./src/osal_task.c) |
| 事件组 | [osal_internal_event.h](./inc/osal_internal_event.h) | [osal_event.c](./src/osal_event.c) |
| 队列 | [osal_internal_queue.h](./inc/osal_internal_queue.h) | [osal_queue.c](./src/osal_queue.c) |
| 互斥量 | [osal_internal_mutex.h](./inc/osal_internal_mutex.h) | [osal_mutex.c](./src/osal_mutex.c) |
| 信号量 | [osal_internal_sema.h](./inc/osal_internal_sema.h) | [osal_sema.c](./src/osal_sema.c) |
| 软件定时器 | [osal_internal_timer.h](./inc/osal_internal_timer.h) | [osal_timer.c](./src/osal_timer.c) |
| 堆内存 | [osal_internal_heap.h](./inc/osal_internal_heap.h) | [osal_heap.c](./src/osal_heap.c) |

任务通知通过 `osal_task_notify()`、`osal_task_notify_wait()` 等接口完成一对一任务唤醒；事件组通过 `osal_event_group_set_bits()` 和 `osal_event_group_wait_bits()` 管理多个事件位。
