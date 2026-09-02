# 看门狗

[← 返回库的目录](../MOC.md) | [← 主页](../../index.md)

## 什么功能

- **软件看门狗表**：最多 8 个任务各自注册超时时间，监控线程周期性扫描，谁超时谁触发复位；
- **硬件 IWDG 兜底**：软件监控线程自己挂掉时，约 5 秒无人喂狗由硬件强制复位；
- **连续复位计数**：用 RTC 备份寄存器记录连续 IWDG 复位次数，达到阈值（默认 3）进入安全模式，防止无限重启；
- **低功耗联动**：进 STOP 前 `pause()` 暂停软件检测，唤醒后 `resume()` 恢复，睡眠期间仍喂硬件狗。

## 文件结构

| 文件                                    | 作用                                 | 移植时                   |
| --------------------------------------- | ------------------------------------ | ------------------------ |
| `watchdog_config.h`                   | 可调宏（任务数上限、阈值、扫描周期） | 按需改                   |
| `service_watchdog_monitor.h` / `.c` | 软件看门狗表 + 复位计数（芯片无关）  | **不改**           |
| `watchdog_adapter_port/`              | 硬件 IWDG / 备份寄存器 / 复位原因    | **换芯片只改这里** |

## 如何使用

```c
#include "service_watchdog_monitor.h"

/* 1. 启动阶段初始化一次（内部会初始化硬件 IWDG） */
watchdog_service_init();

/* 2. 创建监控线程（优先级建议高于普通业务任务） */
/*    在 user_task_reso_config.c 的 g_user_tasks[] 里注册 server_watchdog_task */

/* 3. 各业务任务入口注册自己 */
watchdog_register(osal_task_get_current(), 5000U, "SensorTask");

/* 4. 业务任务主循环里周期喂 */
watchdog_feed(osal_task_get_current());

/* 5. 长时间阻塞等待前注销，避免被误判卡死 */
watchdog_unregister(osal_task_get_current());
```

典型使用模式：

| 场景           | 做法                                                          |
| -------------- | ------------------------------------------------------------- |
| 周期循环型任务 | 入口 register + 循环 feed                                     |
| 长耗时阻塞任务 | 阻塞前 unregister，恢复后 register                            |
| 低功耗         | 进 STOP 前 `watchdog_pause()`，唤醒后 `watchdog_resume()` |

## 如何移植到其它芯片

**只需重写 `watchdog_adapter_port/watchdog_adapter_port.c` 一个文件**，实现 5 个接口：

| 接口                                     | 语义                   | STM32F411 实现                               |
| ---------------------------------------- | ---------------------- | -------------------------------------------- |
| `watchdog_port_init()`                 | 初始化硬件看门狗       | `HAL_IWDG_Init`，64 分频 + 2500 重装 ≈ 5s |
| `watchdog_port_refresh()`              | 喂硬件狗               | `HAL_IWDG_Refresh`                         |
| `watchdog_port_boot_fail_read/write()` | 掉电保留的复位计数     | `RTC->BKP0R`（VBAT 备份域）                |
| `watchdog_port_is_wdg_reset()`         | 判断本次是否 IWDG 复位 | `RCC->CSR & IWDGRSTF`                      |
| `watchdog_port_system_reset()`         | 软复位                 | `NVIC_SystemReset`                         |

移植要点：

1. **备份寄存器**：目标芯片若无备份寄存器，可用内部 Flash 模拟（注意擦写寿命），或外挂 EEPROM 替代 `boot_fail_read/write` 的实现；
2. **复位原因**：目标芯片需提供“本次复位是否由看门狗引起”的判断寄存器；
3. **喂狗周期**：LSI 有 17~47 kHz 偏差，任务超时和喂狗周期要留足裕量，别贴着标称值 5 秒设计。

## 依赖

- `osal.h`（OS 抽象层：task handle / tick / mutex / delay）—— 来自本库 [OSAL 公开接口](../代码架构/firmware/04_platform/platform_os/inc/MOC.md)，即 `firmware/04_platform/platform_os`，工程内自带；
- STM32 HAL 库（仅在 `watchdog_adapter_port.c` 里使用）。
