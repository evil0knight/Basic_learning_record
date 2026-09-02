#ifndef SERVICE_WATCHDOG_MONITOR_H
#define SERVICE_WATCHDOG_MONITOR_H

#include "osal.h"
#include <stdbool.h>
#include <stdint.h>

/*
 * 看门狗监控服务接口
 *
 * 软件看门狗表 + 硬件 IWDG 兜底 + 连续复位计数。
 * 业务任务只需 register/feed/unregister 三个接口。
 */

/* 监控线程入口，需要在 user_task_reso_config.c 的 g_user_tasks[] 中注册 */
void server_watchdog_task(void *argument);

/* 初始化软件看门狗表和复位计数，应在任务注册前调用一次 */
void watchdog_service_init(void);

/* 注册一个任务到软件看门狗表 */
void watchdog_register(osal_task_handle_t handle, uint32_t timeout_ms, const char *name);

/* 从软件看门狗表注销一个任务（长时间阻塞等待前调用） */
void watchdog_unregister(osal_task_handle_t handle);

/* 喂一个已注册任务对应的看门狗表项 */
void watchdog_feed(osal_task_handle_t handle);

/* 暂停软件超时检测（进入 STOP/低功耗前） */
void watchdog_pause(void);

/* 恢复软件超时检测（唤醒后） */
void watchdog_resume(void);

/* 查询是否处于暂停状态 */
bool watchdog_is_paused(void);

/* 查询是否处于安全模式（连续 IWDG 复位次数超阈值） */
bool watchdog_is_safe_mode(void);

/* 清除 boot fail 计数（系统稳定运行后） */
void watchdog_clear_boot_fail_count(void);

#endif /* SERVICE_WATCHDOG_MONITOR_H */
