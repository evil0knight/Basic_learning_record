/**
 * @file user_isr_handlers.c
 * @brief 定义 HAL 中断回调指针。
 *
 * 具体 Driver 回调由设备 Handler 在初始化时挂载。
 */
#include "user_isr_handlers.h"

#include <stddef.h>

user_isr_callback_t pf_pin_interrupt_callback = NULL;
user_isr_callback_t pf_dma_interrupt_callback = NULL;
user_isr_callback_t pf_int_interrupt_callback = NULL;