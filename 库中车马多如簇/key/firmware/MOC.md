# firmware

[← MOC](../MOC.md) | [← 主页](../../../index.md)

---

| 文件                                                                                                                                                                                                  | 作用                               |
| ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------- |
| [user_isr_handlers.h](01_app/app_isr_handlers/user_isr_handlers.h) / [user_isr_handlers.c](01_app/app_isr_handlers/user_isr_handlers.c)                                                                 | PA0 中断向按键队列投递消息         |
| [user_task_reso_config.h](01_app/app_init/user_task_config/user_task_reso_config.h) / [user_task_reso_config.c](01_app/app_init/user_task_config/user_task_reso_config.c)                               | 任务配置表、按键队列创建与句柄获取 |
| [service_5ms_cycle.h](02_service/service_5ms_cycle/service_5ms_cycle.h) / [service_5ms_cycle.c](02_service/service_5ms_cycle/service_5ms_cycle.c)                                                       | 每轮检测按键，随后延时 5ms         |
| [shutdown_monitor.h](02_service/service_5ms_cycle/module_task/shutdown_monitor/shutdown_monitor.h) / [shutdown_monitor.c](02_service/service_5ms_cycle/module_task/shutdown_monitor/shutdown_monitor.c) | 低电平计数、松开清零、长按关机     |
| [rtos_config.h](03_config/rtos_config.h)                                                                                                                                                               | 5ms 任务栈深度与优先级             |

## 接入

1. 配置 PA0 为下降沿 EXTI，按下为低电平；提供 `main.h` 中的 `PA0_USER_KEY3_GPIO_Port`、`PA0_USER_KEY3_Pin` 和 IRQ 分发。
2. 提供 OSAL、GPIO Port 和 HP4570 电源接口；`osal_queue_send()` 须支持 ISR 调用，中断优先级须符合 RTOS 要求。
3. 将源码加入编译列表，添加各模块及外部依赖的头文件目录；同名资源表和 HAL 回调合并使用。
4. 在启动阶段调用一次 `user_os_resources_init()` 并检查返回值，再按 `g_st_user_tasks` 创建任务。队列须在按键中断使用前创建。
5. 已有 5ms 任务时，每轮调用一次 `user_shutdown_monitor_5ms()`，不重复创建扫描任务。

## 工作方式

```text
PA0 下降沿 -> 中断入队 -> 5ms 任务取消息 -> 开始检测
    -> 低电平累计 200 次 -> 使能电源按键唤醒 -> 关闭电池 FET
    -> 松开清零
```

队列深度为 1，消息为 `uint8_t`。收到消息才开始检测；开始后即使队列为空也继续检测电平。队列满时丢弃新事件。

长按门限为 200 次检测，按 5ms 间隔约为 1 秒，实际受任务耗时和调度影响。达到门限后持续按住，每轮都会调用 `hp4570_set_powerkey_wk_en(1)` 和 `hp4570_set_battery_fet_dis(1)`。

`user_shutdown_monitor_5ms()` 返回检测流程状态，不表示电源操作成功。当前按键流程不包含消抖、单击或双击识别。
