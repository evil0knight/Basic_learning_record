/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.h
 *
 * @par dependencies
 * - stddef.h
 *
 * @author YA
 *
 * @brief 声明 HAL 中断回调指针。
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
#ifndef FRIMWARE_BARE_USER_ISR_HANDLERS_H
#define FRIMWARE_BARE_USER_ISR_HANDLERS_H

#include <stddef.h>

typedef void (*user_isr_callback_t)(void *p_object, void *p_data);

extern user_isr_callback_t pf_pin_interrupt_callback;
extern user_isr_callback_t pf_dma_interrupt_callback;
extern user_isr_callback_t pf_int_interrupt_callback;

#endif /* FRIMWARE_BARE_USER_ISR_HANDLERS_H */
