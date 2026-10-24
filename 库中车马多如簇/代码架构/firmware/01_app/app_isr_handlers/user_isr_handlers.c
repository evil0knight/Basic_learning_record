/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.c
 *
 * @par dependencies
 * - user_isr_handlers.h
 *
 * @author YA
 *
 * @brief 定义 HAL 中断回调指针。
 *
 * Processing flow:
 *
 * Handler 挂载回调 -> HAL 中断入口调用回调。
 *
 * @version V1.0 2026-10-24
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#include "user_isr_handlers.h"

#include <stddef.h>

user_isr_callback_t pf_pin_interrupt_callback = NULL;
user_isr_callback_t pf_dma_interrupt_callback = NULL;
user_isr_callback_t pf_int_interrupt_callback = NULL;
