# app_init

[返回 firmware](../../MOC.md) | [返回代码架构](../../../MOC.md) | [返回主页](../../../../../index.md)

## 关键文件

- [user_init.h](./user_init.h) / [user_init.c](./user_init.c)：创建应用初始化任务，初始化 OSAL 资源并根据任务配置表创建应用任务；关键函数为 `userTaskInitFunction()`。
- [user_periph_setup.h](./platform_io_register/user_periph_setup.h) / [user_periph_setup.c](./platform_io_register/user_periph_setup.c)：提供平台外设注册入口；关键函数为 `app_periph_init()`。
- [user_task_reso_config.h](./user_task_config/user_task_reso_config.h) / [user_task_reso_config.c](./user_task_config/user_task_reso_config.c)：维护任务、队列、互斥量、信号量和事件组配置，并提供 OSAL 资源创建和句柄访问接口。

## 调用顺序

```text
main.c
    -> app_periph_init()
    -> UserAppTask_Init()
    -> userTaskInitFunction()
        -> user_os_resources_init()
        -> 遍历任务配置表创建任务
        -> 删除初始化任务
    -> osal_task_start_scheduler()
```
