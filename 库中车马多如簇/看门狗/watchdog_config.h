/*****************************************************************************
 * Copyright (C) 2026 看门狗模块贡献者。
 *
 * 保留所有权利。
 *
 * @file watchdog_config.h
 *
 * @par dependencies
 * - 无。
 *
 * @author YA
 *
 * @brief 定义看门狗服务的容量、超时和复位计数阈值。
 *
 * 处理流程：
 * 由监控服务包含，在编译时使用配置常量。
 *
 * @version V1.0
 *
 * @note 1 个制表符等于 4 个空格；注释采用 UTF-8 中文。
 *
 ****************************************************************************/

#ifndef WATCHDOG_CONFIG_H
#define WATCHDOG_CONFIG_H

//****************************** Defines begin *******************************//
/* 最多同时监控的任务数。 */
#define MAX_WATCHDOG_TASKS        (8U)

/* 空闲表项的默认超时，单位毫秒；注册时由入参覆盖。 */
#define WATCHDOG_TIMEOUT_MS       (10000U)

/* 启动失败计数达到此阈值时，置安全模式标志。 */
#define BOOT_FAIL_THRESHOLD       (3U)

/* 每轮扫描后的延时，单位毫秒。 */
#define WATCHDOG_SCAN_PERIOD_MS   (100U)

/* 任务名称缓冲区长度，包含字符串终止符。 */
#define WATCHDOG_TASK_NAME_LEN    (16U)
//******************************* Defines end ********************************//

#endif

//******************************* End of File ********************************//
