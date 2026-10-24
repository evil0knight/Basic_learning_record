/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.c
 *
 * @par dependencies
 * - user_isr_handlers.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief HAL 中断回调指针定义。
 *
 * Processing flow:
 *
 * Handler 在初始化时将 Driver 回调写入对应指针，HAL 回调通过指针调用 Driver。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "user_isr_handlers.h"

#include <stddef.h>

user_isr_callback_t pf_pin_interrupt_callback = NULL;
user_isr_callback_t pf_dma_interrupt_callback = NULL;
user_isr_callback_t pf_int_interrupt_callback = NULL;


