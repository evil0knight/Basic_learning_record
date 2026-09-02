# OSAL os_implementation

[← 代码架构库](../../../../MOC.md) | [← 嵌入式工程架构](../../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../../MOC.md) | [← 主页](../../../../../../index.md)

---

把 OSAL 内部接口转换为具体操作系统 API。当前实现挂载 FreeRTOS，更换操作系统时替换这一层。

| 资源 | FreeRTOS实现 |
| --- | --- |
| 公共适配 | [os_freertos.h](./freertos/inc/os_freertos.h) |
| 任务 | [os_impl_task.c](./freertos/src/os_impl_task.c) |
| 队列 | [os_impl_queue.c](./freertos/src/os_impl_queue.c) |
| 互斥量 | [os_impl_mutex.c](./freertos/src/os_impl_mutex.c) |
| 信号量 | [os_impl_sema.c](./freertos/src/os_impl_sema.c) |
| 软件定时器 | [os_impl_timer.c](./freertos/src/os_impl_timer.c) |
| 堆内存 | [os_impl_heap.c](./freertos/src/os_impl_heap.c) |
