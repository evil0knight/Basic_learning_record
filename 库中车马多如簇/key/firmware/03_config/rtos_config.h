/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc. or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file rtos_config.h
 * @par dependencies
 * - 无。
 * @author YA
 * @brief 配置 5ms 任务的栈深度和优先级。
 *
 * Processing flow:
 * App 任务配置表读取宏 -> OSAL 创建任务。
 *
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef KEY_RTOS_CONFIG_H
#define KEY_RTOS_CONFIG_H

/* 任务栈深度，单位遵循目标 OSAL。 */
#define CFG_TASK_5MS_STACK_SIZE 256U

/* 任务优先级，按目标工程的调度需求配置。 */
#define CFG_TASK_5MS_PRIORITY 1U

#endif /* KEY_RTOS_CONFIG_H */
