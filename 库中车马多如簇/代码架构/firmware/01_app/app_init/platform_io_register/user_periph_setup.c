/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_periph_setup.c
 *
 * @par dependencies
 * - user_periph_setup.h
 *
 * @author YA
 *
 * @brief App 层平台适配器注册实现。
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
#include "user_periph_setup.h"

/**
 * @brief 注册 App 使用的平台适配器。
 *
 * Steps:
 *  1. 保留统一注册入口。
 *  2. 具体外设适配器由目标工程按需添加。
 *
 * @param[in] None
 * @return void : 无返回值。
 */
void app_periph_init(void)
{
    /* 模板不绑定具体外设。 */
}
