# App 初始化

[← firmware](../../MOC.md) | [← 嵌入式工程架构](../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../MOC.md) | [← 主页](../../../../../index.md)

---

## 关键文件

- [user_init.h](./user_init.h) / [user_init.c](./user_init.c)：
  创建 userTask 初始化任务。userTask 内部先初始化 OSAL 资源，再遍历任务配置表创建任务，最后删除自身。

- [user_periph_setup.h](./platform_io_register/user_periph_setup.h) / [user_periph_setup.c](./platform_io_register/user_periph_setup.c)：
  提供 app_periph_init() 平台适配器注册入口，由 main.c 在 UserAppTask_Init() 前调用。

- [user_task_reso_config.h](./user_task_config/user_task_reso_config.h) /
  [user_task_reso_config.c](./user_task_config/user_task_reso_config.c)：
  维护任务、队列、互斥量、信号量和事件组配置表，负责 OSAL 资源创建、释放和句柄获取。任务通知直接使用任务句柄，不单独配置资源。

- [user_isr_handlers.h](../app_isr_handlers/user_isr_handlers.h) /
  [user_isr_handlers.c](../app_isr_handlers/user_isr_handlers.c)：
  提供 App 中断回调注册和事件分发入口，不绑定具体芯片 HAL。

## 调用顺序

~~~text
main.c
 ├─ app_periph_init()
 ├─ UserAppTask_Init()
 │   └─ userTaskInitFunction()
 │       ├─ user_os_resources_init()
 │       ├─ 遍历任务配置表创建任务
 │       └─ 删除初始化任务
 └─ osal_task_start_scheduler()
~~~
