# firm_bare

[← MOC](../MOC.md) | [← 主页](../../../index.md)

---

| 文件 | 作用 |
| --- | --- |
| [bsp_key_types.h](05_bsp/bsp_key/bsp_key_types.h) | 状态码、动作和中断回调类型 |
| [bsp_key_driver.h](05_bsp/bsp_key/driver/bsp_key_driver.h) / [bsp_key_driver.c](05_bsp/bsp_key/driver/bsp_key_driver.c) | 读取 GPIO，转换有效电平 |
| [bsp_key_handler.h](05_bsp/bsp_key/handler/bsp_key_handler.h) / [bsp_key_handler.c](05_bsp/bsp_key/handler/bsp_key_handler.c) | 持有 Driver 和 Handler 静态对象，处理消抖、短按和长按 |
| [bsp_adapter_port_key.h](04_platform/platform_bsp/bsp_adapter/bsp_adapter_port_key.h) / [bsp_adapter_port_key.c](04_platform/platform_bsp/bsp_adapter/bsp_adapter_port_key.c) | 按键配置表、平台接口和对象挂载 |
| [bsp_wrapper_key.h](04_platform/platform_bsp/bsp_wrapper/bsp_wrapper_key.h) / [bsp_wrapper_key.c](04_platform/platform_bsp/bsp_wrapper/bsp_wrapper_key.c) | 挂载、扫描、读取事件、中断通知和解除 |
| [user_init.h](01_app/app_init/user_init.h) / [user_init.c](01_app/app_init/user_init.c) | 初始化 App 子模块，挂载按键 |
| [app_control.h](01_app/app_control/app_control.h) / [app_control.c](01_app/app_control/app_control.c) | App 事件枚举和应用状态转移表 |
| [user_isr_handlers.h](01_app/app_isr_handlers/user_isr_handlers.h) / [user_isr_handlers.c](01_app/app_isr_handlers/user_isr_handlers.c) | HAL EXTI 回调和框架回调指针 |
| [app_loop.h](01_app/app_loop/app_loop.h) / [app_loop.c](01_app/app_loop/app_loop.c) | 调用 Wrapper 扫描和读取事件，映射 App 事件并入队 |

## 配置

按键配置集中在 Adapter 的 `g_a_key_pins[]`，数量由表长度计算，最多 16 键。增加按键时添加表项，ID 从 0 连续编号；每键单独设置 GPIO、有效电平、扫描、消抖和长按时间。

```c
static const st_key_pin_config_t g_a_key_pins[] =
{
    /* 端口、引脚、高电平有效、ID、扫描ms、消抖ms、长按ms。 */
    {KEY0_GPIO_Port, KEY0_Pin, false, {0U, 5U, 15U, 1000U}}
};
```

同步在 `app_control.h` 增加对应事件枚举，在 `app_loop.c` 的 `g_a_key_events[]` 增加动作映射，表索引与按键 ID 一致。各键须使用不同 EXTI 线；重复引脚、非法掩码或 ID 会在初始化时被拒绝。

## 接入

1. CubeMX 配置 `KEY0` 引脚及 EXTI。默认低电平按下，使用下降沿中断和匹配电路的上拉；修改有效电平时同步修改触发边沿。
2. 将模块源码加入编译，各头文件目录加入搜索路径；提供 `main.h`、`HAL_GetTick()`、GPIO Port、Critical Port 和架构已有的 `msg_queue.h/.c`、`msg_dispatcher.h/.c`。事件队列使用这里的 `app_control.h` 类型定义。
3. App 文件沿用架构的文件名与入口。目标工程已有同名文件时，合并按键相关内容，保留原有设备和业务，不重复编译同名模块。
4. GPIO 和毫秒时基就绪后调用 `UserAppInit()` 并检查返回值。它先初始化队列、控制层、主循环，再调用 `drv_adapter_key_register()` 完成按键挂载。初始化期间可屏蔽按键 EXTI，成功后清除遗留中断并启用。
5. 主循环继续并列调用 `do_read_message()` 和 `do_loop()`；`do_loop()` 直接调用 Wrapper 扫描和读取事件，映射为 App 事件后入队。HAL EXTI 回调调用 `bsp_wrapper_key_irq_notify()`，合并回调时保留其他设备分支。
6. 在 `app_control.c` 的状态转移表中为需要的按键事件填写 `pf_action`，由动作调用 Service。表中默认 `NULL` 不执行业务，会返回 `APP_STATUS_NO_TRANSITION`。

```text
UserAppInit
    -> 初始化已有 App 子模块
    -> drv_adapter_key_register()
    -> Adapter 获取 Handler 层静态对象，绑定平台接口和中断回调
    -> Wrapper 复制设备描述，供主循环直接调用

HAL EXTI -> Wrapper irq_notify -> Adapter 按引脚查找 -> Driver pending
主循环 do_loop
    -> Wrapper scan -> Handler 检测并保存事件位
    -> Wrapper take_events -> 读取并清除事件位
    -> App 映射事件 -> queue_push
    -> do_read_message -> 状态转移表 -> 业务 Service
```

`drv_adapter_key_register()` 无入参。Handler 层持有一个 Handler 和 Driver 静态数组；每键 Driver 保存独立配置、消抖、长按及事件状态。Adapter 绑定平台接口、配置和设备操作。Wrapper 保存设备描述副本，`p_user_data` 指向 Adapter 平台对象数组。挂载后直接扫描，无需单独的初始化入口。BSP 不调用应用队列。

中断挂载和解除在 Adapter 内完成。短临界区仅保护 ISR 通知标志的读取清零，以及回调与对象的挂载、解除，退出时恢复原中断状态。扫描和事件位读取都在主循环中执行；事件位不被 ISR 访问，无需临界区。App 队列内部的并发保护由队列模块负责。启动时调用一次 `UserAppInit()` 完成挂载。

## 检测规则

- 每 5ms 扫描活动键，按下和松开分别稳定 15ms 后确认；采样电平变化则重新消抖。空闲键不持续读取 GPIO，上电已按住也进入检测。
- 整组共用时间和临界区接口；全部空闲时直接返回。有活动键时，一次临界区批量取走通知，一次读取时间供本轮各键使用，GPIO 按各键采样间隔分别读取。
- 确认按下记录 `PRESS` 事件位并开始计时；按住满 1000ms 记录一次 `LONG_PRESS`。重复中断不重置计时，松开消抖期间不触发长按。
- 确认松开记录 `RELEASE`；没有触发长按则同时记录 `SHORT_PRESS`，随后回到空闲。
- 主循环和事件动作须非阻塞。时间差支持毫秒计数回绕，各门限非零且小于 `2^31ms`；长时间漏采样不能通过补跑恢复。
- 每个动作对应一个事件位，`bsp_wrapper_key_take_events()` 读取后清零；主循环每次扫描后立即取走。事件位不保存次数和历史顺序，连续扫描而不读取会合并未读取的同类事件。
- GPIO 读取失败不推进该键状态；当前 GPIO Port 只返回电平，不能报告硬件读取错误。主循环对已取出的事件逐项入队，失败不重发，通过返回值报告首个扫描、读取或入队错误。
- 停止轮询后调用 `bsp_wrapper_key_deinit()` 解除挂载；成功后可再次调用 `drv_adapter_key_register()` 挂载。
