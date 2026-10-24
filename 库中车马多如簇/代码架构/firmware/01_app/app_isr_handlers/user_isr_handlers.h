/**
 * @file user_isr_handlers.h
 * @brief 声明 HAL 中断回调指针。
 *
 * HAL 回调直接调用本文件声明的设备回调指针。
 */
#ifndef FIRMWARE_USER_ISR_HANDLERS_H
#define FIRMWARE_USER_ISR_HANDLERS_H

#include <stddef.h>

typedef void (*user_isr_callback_t)(void *p_object, void *p_data);

extern user_isr_callback_t pf_pin_interrupt_callback;
extern user_isr_callback_t pf_dma_interrupt_callback;
extern user_isr_callback_t pf_int_interrupt_callback;

#endif /* FIRMWARE_USER_ISR_HANDLERS_H */