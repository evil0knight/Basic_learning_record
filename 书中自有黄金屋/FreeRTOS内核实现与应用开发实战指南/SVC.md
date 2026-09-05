# SVC：从用户态翻进内核态的门

[← 链表和调度](./链表和调度.md) | [← FreeRTOS内核实现与应用开发实战指南](./MOC.md)

---

挂载任务、读写队列、取放信号量——这些动的都是**内核的全局链表和数据结构**，属于**内核级操作**。任务代码能不能直接干，取决于它跑在[特权还是非特权](../../术中自有万钟粟/Cortex-M4内核原理/操作模式和状态.md)线程模式；要不要进 **SVC**，取决于这两级之间有没有被 MPU 焊死一堵墙。

## 哪些算"内核级操作"

凡是会碰到 **OS 私有的控制块、链表头数组、寄存器** 的动作都算，不止挂链表这一种：

| 类别 | 入口 | 动的是内核的什么 |
| --- | --- | --- |
| 任务创建 / 删除 | `xTaskCreate()` / `vTaskDelete()` | 从堆里分 TCB + 任务栈，`pxPortInitialiseStack()` 预填栈帧 |
| 任务挂载 / 调度 | `prvAddTaskToReadyList()`、`vTaskSwitchContext` | `xStateListItem` 插就绪链表、`pxCurrentTCB`、就绪位图、悬起 PendSV |
| 队列 | `xQueueSend()` / `xQueueReceive()` | 队列控制块的环形缓冲、`xTasksWaitingToSend` / `xTasksWaitingToReceive` 两条事件链表 |
| 信号量（二值 / 计数） | `xSemaphoreGive()` / `xSemaphoreTake()` | 队列的计数值、事件等待链表（信号量本质是 0 长度的队列） |
| 互斥量 | `xSemaphoreCreateMutex()` + Give / Take | 计数值 + 持有者 TCB + **优先级继承**（`vTaskPriorityInherit()` / `vTaskPriorityDisinherit()`） |
| 任务通知 | `xTaskNotify()` / `xTaskNotifyGive()` | 直接改 TCB 的 `ulNotifiedValue` 和通知状态，无需建队列/信号量 |
| 事件组 | `xEventGroupSetBits()` / `xEventGroupWaitBits()` | 事件标志位 `uxEventBits`、等事件任务链表 |
| 栈 / 上下文现场 | SVC / PendSV / 进中断 | 把寄存器现场压进当前任务的栈、任务切换时按 `pxTopOfStack` 弹出恢复 |
| 堆管理 | `pvPortMalloc()` / `vPortFree()` | 空闲链表 / 内存堆，TCB、队列控制块、任务栈的内存都从这出 |

> 共性：改的全是 OS 自己的数据（`pxReadyTasksLists[]`、TCB、队列控制块、堆、`pxCurrentTCB`）或[只许特权访问](../../术中自有万钟粟/Cortex-M4内核原理/特殊寄存器.md)的寄存器（NVIC / CONTROL）。其中任务栈的**压栈 / 弹栈**也归内核管：现场入哪、按什么帧格式恢复（见[函数栈帧与压栈出栈](../../术中自有万钟粟/Cortex-M4内核原理/函数栈帧与压栈出栈.md)），用户业务代码只负责往 API 传参，不该自己碰内核的栈帧布局和链表。这些都不能让普通用户代码随手改。

## 默认 FreeRTOS：不开 MPU，任务天生就是"特权级"

`configENABLE_MPU` 默认关。启动时 `svc 0` 触发 SVC（见 [freertos启动](./freertos启动.md) 的 `prvStartFirstTask`→`vPortSVCHandler`），清 `CONTROL.nPRIV`，第一个任务以**特权线程模式**开跑，此后每个任务都特权。

- 内核数据结构**没有硬件隔离**：任务要挂链表、读队列、取信号量，直接操作就行，无需抬权限
- 所以 `xQueueSend()` / `xSemaphoreTake()` 等 API 都是**普通函数调用**，内部直接怼链表，**不进 SVC**——本来就在特权级，没墙可翻
- 此时 SVC 只在启动阶段露一次面，日常根本用不到

## 开了 MPU：任务被摁成"非特权用户态"，内核操作只能走 SVC

`configENABLE_MPU` 打开后，MPU 把内存划分 region 并按 AP 位设权限（见 [MPU](../../术中自有万钟粟/Cortex-M4内核原理/MPU.md)）：

| region | AP | 谁可访问 |
| --- | --- | --- |
| 内核代码 / 内核数据（就绪链表、TCB、队列控制块） | 仅特权 | 内核 |
| 任务自己的栈 / 代码 | 特权 + 非特权 | 用户任务 |

- 任务以 `CONTROL.nPRIV = 1` 的**非特权线程模式**运行 = 真·用户态
- 此时任务想"挂载任务 / 读队列 / 取信号量"这类内核操作 = 想碰一块非特权访问不了的 region → 直接权限 fault

解决：这些 API 内部包一层**系统调用**，执行 `svc` 指令软中断，陷入 **SVC handler**。SVC 跑在**处理模式，天然特权**，于是完成真正的链表挂载 / 队列读写，再异常返回用户态继续跑——这就是"从用户态转变到内核态"：

```
用户任务(非特权线程模式)
   │ 调 xQueueSend() → 内部 svc 指令      ← 用户态,碰不到内核数据
   ▼
SVC handler(处理模式,特权)                ← 陷入,内核态
   │ 真正挂链表、读写队列控制块
   ▼
异常返回(bx r14)
   ▼
任务继续执行(回到用户态)
```

> 这套机制正是 ARM 给 OS 留 SVC 的意义：**非特权代码永远不能自己抬权限**，只能靠"触发异常 → 进处理模式干特权活 → 返回"这一条路（CONTROL.nPRIV 只在特权下能写，见 [操作模式和状态](../../术中自有万钟粟/Cortex-M4内核原理/操作模式和状态.md)、[特殊寄存器](../../术中自有万钟粟/Cortex-M4内核原理/特殊寄存器.md)）。
