/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_init.h
 *
 * @par dependencies
 * - osal.h
 *
 * @author YA
 *
 * @brief App 层初始化任务接口。
 *
 * Processing flow:
 *
 * main.c -> app_periph_init -> UserAppTask_Init ->
 * userTaskInitFunction -> user_os_resources_init -> 任务配置表循环创建。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef USER_INIT_H
#define USER_INIT_H

#include "osal.h"

/**
 * @brief 创建 App 初始化任务。
 *
 * Steps:
 *  1. 创建统一的 App 初始化任务。
 *  2. 由初始化任务申请资源并启动已启用的业务任务。
 *
 * @param[in] None
 * @return void : 无返回值。
 */
void UserAppTask_Init(void);

#endif /* USER_INIT_H */
