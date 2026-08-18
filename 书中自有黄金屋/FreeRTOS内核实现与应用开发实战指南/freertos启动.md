# FreeRTOS 启动

[← 任务插入链表和调度](./任务插入链表和调度.md) | [← FreeRTOS内核实现与应用开发实战指南](./MOC.md)

---

启动里要用到 ARM 专门给操作系统准备的两样东西：**SVC**（Supervisor Call 软中断，跑在 handler 模式、特权级，能安全操作系统寄存器）和 **PSP**（任务专用栈指针，每任务一个，区别于中断用的 MSP）。

## ① 进 prvStartFirstTask：回收 main 的历史栈

`vTaskStartScheduler()` → `xPortStartScheduler()` 里，设好 PendSV/SysTick 最低优先级、启动 SysTick 后，调 `prvStartFirstTask()`：

```asm
prvStartFirstTask
    ldr r0, =0xE000ED08     ; VTOR（向量表偏移寄存器）地址
    ldr r0, [r0]            ; 取向量表起始地址
    ldr r0, [r0]            ; 取向量表第一个字 = 初始 MSP 栈顶
    msr msp, r0             ; MSP 复位 → 回收 main 的历史栈
    mov r0, #0
    msr control, r0         ; 清 CONTROL：特权 Thread 模式 + 用 MSP
    cpsie i                 ; 全局开中断
    cpsie f                 ; 开 fault 异常
    dsb
    isb
    svc 0                   ; 触发 SVC
    nop
    nop
```

把 MSP 拉回向量表里的初始值，main 的栈帧全被覆盖。反正后面就进操作系统 SVC、跑调度任务区了，这里已经不是裸机，main 函数直接报废。

## ② 进 SVC 中断：执行专门为 RTOS 准备的启动函数

上面 `svc 0` 触发 SVC 中断，跳进 `vPortSVCHandler`——这个 handler 就是专门为启动第一个任务写的：

```asm
vPortSVCHandler
    ldr r3, =pxCurrentTCB   ; 取 pxCurrentTCB 的地址
    ldr r1, [r3]            ; 取 pxCurrentTCB（指向 TCB）
    ldr r0, [r1]            ; 取 TCB 第一个成员 pxTopOfStack（栈顶）
    ldmia r0!, {r4-r11, r14} ; 把第一个任务要弹到 CPU 的寄存器全弹出来
    msr psp, r0             ; PSP 指向剩下的栈帧
    isb
    mov r0, #0
    msr basepri, r0         ; 解除中断屏蔽
    bx r14                  ; 异常返回
```

`pxTopOfStack` 是「任务(线程)的定义」里 `pxPortInitialiseStack` 按异常返回格式预填的栈顶。SVC 里先手动弹出一段（r4-r11 + r14），`bx r14` 时 r14 = **0xFFFFFFFD**（EXC_RETURN："返回到 Thread 模式 + 用 PSP"），硬件再自动弹出剩余异常帧（r0-r3、r12、LR、xPSR）。

## ③ PC 跳转运行

硬件弹异常帧时，**PC 被装上任务入口函数地址**，于是直接用 PC 跳转，第一个任务开跑：

| 地址 | 内容 | 弹出方 |
| --- | --- | --- |
| 高 | xPSR / **PC(任务入口)** / LR / R12~R1 / R0(任务参数) | 硬件（`bx r14` 后自动弹） |
| ↓ | R14 = EXC_RETURN(0xFFFFFFFD) | SVC 手动 `ldmia` |
| 低 | R11 ~ R4 | SVC 手动 `ldmia` |
