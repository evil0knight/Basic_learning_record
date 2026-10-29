# 01_app

[返回 frimware_bare](../MOC.md) | [返回代码架构](../../MOC.md) | [返回主页](../../../../index.md)

## 关键文件

- [user_init.h](./app_init/user_init.h) / [user_init.c](./app_init/user_init.c)：(需要在接入新子模块时追加 init 调用)
  裸机 App 层统一初始化入口；`UserAppInit()` 由 `main` 在进入 `while(1)` 前调用，依次初始化事件队列、状态机、主循环 ISR 回调，后续子模块 init 顺序接入。
- [app_control.h](./app_control/app_control.h) / [app_control.c](./app_control/app_control.c)：(需要添加事件状态转移表)
  定义应用状态、事件和状态转移表，并维护当前状态；
  `app_control_event_map()` 按事件查表并执行已接入的 `pf_action`，`app_control_init()` 初始化状态，`app_control_get_current()` 读取当前状态。
- [msg_queue.h](./app_message/msg_queue.h) / [msg_queue.c](./app_message/msg_queue.c)：(不需要添加)
  实现固定容量环形事件队列；
  `queue_init()` 初始化，`queue_push()` 写入 `app_event_t`，`queue_pop()` 取出事件。
- [msg_dispatcher.h](./app_message/msg_dispatcher.h) / [msg_dispatcher.c](./app_message/msg_dispatcher.c)：(不需要添加)
  排空事件队列并调用 `app_control_event_map()`；`do_read_message()` 负责主循环事件处理。
- [app_loop.h](./app_loop/app_loop.h) / [app_loop.c](./app_loop/app_loop.c)：(需要添加后台轮询与 ISR 处理)
  主循环后台轮询与定时器 ISR 回调入口；`app_loop_init()` 保留初始化入口，`do_loop()` 由主循环调用，`do_loop_tick()` 由定时器 ISR 调用。
- [user_isr_handlers.h](./app_isr_handlers/user_isr_handlers.h) / [user_isr_handlers.c](./app_isr_handlers/user_isr_handlers.c)：(需要添加中断回调函数)
  实现 HAL 回调并直接调用 Platform 事件接口；定时器 HAL 回调可直接调用 `do_loop_tick()`，设备事件由 Platform/Driver 处理或投递队列。

## 调用顺序

```text
main
    -> UserAppInit()          // 队列 / 状态机 / 主循环 ISR 回调等 init
    -> while(1) { do_read_message(); do_loop(); }
```

```text
HAL 中断回调
    -> Platform 事件处理接口
    -> Driver 中断处理
    -> queue_push(EVENT_xxx)
    -> 主循环 do_read_message()
    -> queue_pop()
    -> app_control_event_map(EVENT_xxx)
    -> 状态转移表
    -> pf_action
    -> Service
    -> Wrapper / Adapter / Handler / Driver
```

## 定时器 ISR 调用链

```text
HAL_TIM_PeriodElapsedCallback
    -> 检查定时器句柄
    -> do_loop_tick()           （直接调用）
```

主循环 `while` 并列调用：

```text
do_read_message()   // 事件驱动
do_loop()           // 后台轮询（周期关键处理由定时器 ISR 抢占）
```
