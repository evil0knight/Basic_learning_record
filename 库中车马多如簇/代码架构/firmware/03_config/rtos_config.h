/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file rtos_config.h
 *
 * @par dependencies
 * - 无。
 *
 * @author YA
 *
 * @brief RTOS 任务、队列和信号量资源配置。
 *
 * Processing flow:
 *
 * OS 初始化阶段根据这些宏创建任务和同步资源。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

/* 任务栈大小和优先级。 */
#define CFG_TASK_XXX_STACK_SIZE 256U
#define CFG_TASK_XXX_PRIORITY   1U

/* 队列和信号量资源参数。 */
#define CFG_QUEUE_XXX_DEPTH     1U
#define CFG_SEMA_XXX_MAX_COUNT  1U
#define CFG_SEMA_XXX_INIT_COUNT 0U

#endif /* RTOS_CONFIG_H */
