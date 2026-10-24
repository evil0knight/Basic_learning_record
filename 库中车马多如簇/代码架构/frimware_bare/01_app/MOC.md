# 01_app

[返回 frimware_bare](../MOC.md) | [返回代码架构](../../MOC.md) | [返回主页](../../../../index.md)

## 关键文件

- [app_control.h](./app_control/app_control.h) / [app_control.c](./app_control/app_control.c)：(需要添加事件状态转移表)
  定义应用状态、事件和状态转移表，并维护当前状态；
  `app_control_event_map()` 按事件查表并执行已接入的 `pf_action`，`app_control_init()` 初始化状态，`app_control_get_current()` 读取当前状态。
- [msg_queue.h](./app_message/msg_queue.h) / [msg_queue.c](./app_message/msg_queue.c)：(不需要添加)
  实现固定容量环形事件队列；
  `queue_init()` 初始化，`queue_push()` 写入 `app_event_t`，`queue_pop()` 取出事件。
- [msg_dispatcher.h](./app_message/msg_dispatcher.h) / [msg_dispatcher.c](./app_message/msg_dispatcher.c)：(不需要添加)
  排空事件队列并调用 `app_control_event_map()`；`do_read_message()` 负责主循环事件处理。
- [user_isr_handlers.h](./app_isr_handlers/user_isr_handlers.h) / [user_isr_handlers.c](./app_isr_handlers/user_isr_handlers.c)：(需要添加中断回调函数)
  保存 HAL 中断回调指针，由 Driver 回调产生 `EVENT_xxx` 并写入事件队列。

## 调用顺序

```text
HAL 中断回调
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
