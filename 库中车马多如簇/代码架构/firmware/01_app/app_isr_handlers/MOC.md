# app_isr_handlers

[← 代码架构库](../../../MOC.md) | [← 嵌入式工程架构](../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../MOC.md) | [← 主页](../../../../../index.md)

---

接收底层中断事件并转交给任务处理，不在中断中执行耗时业务。

| 内容 | 作用 |
| --- | --- |
| [user_isr_handlers.h](./user_isr_handlers.h) | 定义平台无关的中断事件和回调接口 |
| [user_isr_handlers.c](./user_isr_handlers.c) | 保存回调并分发中断事件 |

## user_isr_handlers配置:

1. 在 `user_isr_handlers.h` 添加事件枚举：

```c
typedef enum
{
    USER_ISR_EVENT_XXX = 0,
    USER_ISR_EVENT_MAX
} user_isr_event_t;
```

2. 在业务模块中编写回调：

```c
static void xxx_isr_callback(user_isr_event_t event,
                             void *context,
                             void *argument)
{
    (void)event;
    (void)context;
    (void)argument;
    /* 只发送队列或释放信号量 */
}
```

3. 在初始化阶段注册回调：

```c
user_isr_callback_register(USER_ISR_EVENT_XXX,
                           xxx_isr_callback,
                           NULL);
```

4. 在芯片 HAL 的中断回调中只做事件分发：

```c
user_isr_dispatch(USER_ISR_EVENT_XXX, NULL);
```
