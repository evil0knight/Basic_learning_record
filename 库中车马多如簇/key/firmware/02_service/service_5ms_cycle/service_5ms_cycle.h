/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc. or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file service_5ms_cycle.h
 * @par dependencies
 * - 无。
 * @author YA
 * @brief 声明按键检测的 5ms 任务入口。
 *
 * Processing flow:
 * App 创建任务 -> 每轮执行按键检测 -> 延时 5ms。
 *
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef KEY_SERVICE_5MS_CYCLE_H
#define KEY_SERVICE_5MS_CYCLE_H

/**
 * @brief 运行按键检测的 5ms 任务循环。
 * @param[in] p_argument 未使用，允许为空。
 * @return void OS 任务入口的固定签名，不返回。
 */
void thread_5ms_task(void *p_argument);

#endif /* KEY_SERVICE_5MS_CYCLE_H */
