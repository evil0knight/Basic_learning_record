#ifndef WATCHDOG_CONFIG_H
#define WATCHDOG_CONFIG_H

/* 看门狗服务可调参数（移植时只需改这里） */

/* 软件看门狗表最多同时监控的任务数 */
#define MAX_WATCHDOG_TASKS        (8U)

/* 未显式指定超时时的默认超时(ms) */
#define WATCHDOG_TIMEOUT_MS       (10000U)

/* 连续多少次 IWDG 复位后进入安全模式 */
#define BOOT_FAIL_THRESHOLD       (3U)

/* 监控线程扫描周期(ms) */
#define WATCHDOG_SCAN_PERIOD_MS   (100U)

/* 监控表里任务名的最大长度(含结尾 '\0') */
#define WATCHDOG_TASK_NAME_LEN    (16U)

#endif /* WATCHDOG_CONFIG_H */
