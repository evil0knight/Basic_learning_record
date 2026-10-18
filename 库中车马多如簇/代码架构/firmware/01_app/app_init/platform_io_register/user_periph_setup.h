/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_periph_setup.h
 *
 * @par dependencies
 * - None
 *
 * @author YA
 *
 * @brief App 层平台适配器注册接口。
 *
 * Processing flow:
 *
 * app_periph_init 预留平台适配器注册入口。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef USER_PERIPH_SETUP_H
#define USER_PERIPH_SETUP_H

/**
 * @brief 注册 App 使用的平台适配器。
 *
 * Steps:
 *  1. 保留统一的适配器注册入口。
 *  2. 由具体工程在此处挂载实际外设适配器。
 *
 * @param[in] None
 * @return void : 无返回值。
 */
void app_periph_init(void);

#endif /* USER_PERIPH_SETUP_H */
