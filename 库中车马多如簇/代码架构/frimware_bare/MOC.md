# frimware_bare

[←MOC](../MOC.md) | [←主页](../../../index.md)
[←返回表驱法](../../../术中自有万钟粟/嵌入式工程架构/表驱法.md)

---

## 文件索引

- [app_event.h](app_event.h)
- [app_event_flag.c](app_event_flag.c)
- [app_event_flag.h](app_event_flag.h)
- [app_message.h](app_message.h)
- [app_state.c](app_state.c)
- [app_state.h](app_state.h)
- [hal_uart.c](hal_uart.c)
- [main.c](main.c)
- [msg_dispatcher.c](msg_dispatcher.c)
- [msg_dispatcher.h](msg_dispatcher.h)
- [msg_queue.c](msg_queue.c)
- [msg_queue.h](msg_queue.h)

1. 中断把原始数据读出来，封装成 `EventMessage_t`
2. 中断把消息压进 `g_msgQueue`
3. 中断置位 `APP_EVENT_MESSAGE_READY`
4. 主循环检测标志后调用 `do_read_message()`
5. `do_read_message()` 从队列取消息
6. `msg_dispatcher_dispatch()` 按 `source_id` 查 `HandlerTable`
7. 对应的 `Parse_XXX_Data()` 把原始数据翻译成 `EVENT_START / EVENT_STOP / EVENT_FAULT`
8. `app_state_process_event()` 查 `g_transitionTable`
9. 命中后执行带 `msg` 参数的回调函数

## 关键点

- **信息处理能力**来自这里：

```c
typedef void (*StateAction)(const EventMessage_t *msg);
```

不是 `void (*action)(void)`。

- **消息分发表**在这里：

```c
static const MsgHandler_t HandlerTable[MSG_SRC_MAX]
```

- **状态转移表**在这里：

```c
static const Transition g_transitionTable[]
```

## 新增一路外设时，只改这 4 处

1. `app_message.h` 里给 `MsgSource_t` 加枚举项
2. `msg_dispatcher.c` 里新增一个 `Parse_XXX_Data()`
3. `msg_dispatcher.c` 里给 `HandlerTable` 加一项
4. 对应外设 ISR 里按同样方式封装 `EventMessage_t` 入队

## 新增一个状态事件时，只改这 3 处

1. `app_event.h` 里加事件枚举
2. `app_state.c` 里新增动作函数
3. `app_state.c` 里给 `g_transitionTable` 加一行
