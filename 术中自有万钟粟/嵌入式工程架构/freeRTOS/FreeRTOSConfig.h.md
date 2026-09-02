# FreeRTOSConfig.h

[← 返回 FreeRTOS](./MOC.md) | [← 主页](../../../index.md)

> 官方配置文档（每个 config 参数的含义）：[Customization — FreeRTOS 文档](https://www.freertos.org/Documentation/02-Kernel/03-Supported-devices/02-Customization)
>
> 官方模板（可直接复制改）→ [examples/template_configuration/FreeRTOSConfig.h
> ](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/examples/template_configuration/FreeRTOSConfig.h)

---

## 要详细的看上面的两个引用网址!

**通过这个文件可以:** 

**❗必须手动配置检查    🚫最好别更改    👽常用配置不用管      (除了❗️大部分项目都不用改的)**

1. 👽 可以输出xxx文件xxx行（断言机制 `configASSERT`）
2. 🚫 可以让任务可以抢占式调度（`configUSE_PREEMPTION`）
3. 👽 可以时间片调度（`configUSE_TIME_SLICING`）
4. 🚫 可以用位图加速让调度更快速（`configUSE_PORT_OPTIMISED_TASK_SELECTION`）
5. 👽 可以设置低功耗tickless（`configUSE_TICKLESS_IDLE`）
6. ❗ 可以配置时钟频率（`configCPU_CLOCK_HZ`）
7. 👽 可以配置systick频率（`configTICK_RATE_HZ`）
8. 👽 可以设置最大优先级（`configMAX_PRIORITIES`）
9. 👽 可以配置空闲任务默认使用栈大小（`configMINIMAL_STACK_SIZE`）
10. 👽 可以定义任务字符串最大长度（`configMAX_TASK_NAME_LEN`）
11. 🚫 可以定义系统节拍计数器类型（`configUSE_16_BIT_TICKS`）
12. 👽 可以控制空闲任务放弃cpu使用权给其他优先级的用户任务（`configIDLE_SHOULD_YIELD`）
13. 🚫 可以启用消息队列
14. 🚫 可以开启任务通知
15. 👽 可以开启互斥,递归互斥,计数信号量
16. 👽 可以设置信号量和队列的个数（注册队列/信号量最大数 `configQUEUE_REGISTRY_SIZE`）
17. 🚫 可以支持动态内存分配申请（`configSUPPORT_DYNAMIC_ALLOCATION`）
18. 👽 可以支持静态内存分配（`configSUPPORT_STATIC_ALLOCATION`）
19. ❗ 可以配置内核总计可用的有效RAM大小（`configTOTAL_HEAP_SIZE`）
20. 👽 可以配置空闲任务钩子函数（`configUSE_IDLE_HOOK`）
21. 👽 可以配置时间片节拍钩子函数（`configUSE_TICK_HOOK`）
22. 👽 可以配置内存申请失败钩子函数（`configUSE_MALLOC_FAILED_HOOK`）
23. 👽 可以启用栈溢出检测功能及钩子函数（`configCHECK_FOR_STACK_OVERFLOW`）
24. 👽 可以启用运行时间统计功能（`configGENERATE_RUN_TIME_STATS`）
25. 👽 可以启用可视化跟踪调试功能（`configUSE_TRACE_FACILITY`）
26. 🚫 可以启用协程及配置协程有效优先级数（`configUSE_CO_ROUTINES`、`configMAX_CO_ROUTINE_PRIORITIES`）
27. 👽 可以启用软件定时器（`configUSE_TIMERS`）
28. 👽 可以配置软件定时器任务优先级（`configTIMER_TASK_PRIORITY`）
29. 👽 可以配置软件定时器命令队列长度（`configTIMER_QUEUE_LENGTH`）
30. 👽 可以配置软件定时器任务栈大小（`configTIMER_TASK_STACK_DEPTH`）
31. 👽 可以配置是否裁剪/包含常用 API 函数接口（`INCLUDE_xTaskGetSchedulerState` 等）
32. ❗ 可以定义MCU中断优先级位数（`__NVIC_PRIO_BITS`）
33. ❗ 可以配置SysTick与PendSV使用的系统最低中断优先级（`configLIBRARY_LOWEST_INTERRUPT_PRIORITY`）
34. ❗ 可以配置FreeRTOS可管理的最大中断优先级/BASEPRI（`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`）
35. 🚫 可以对需要配置的SysTick与PendSV中断优先级进行位偏移对齐
