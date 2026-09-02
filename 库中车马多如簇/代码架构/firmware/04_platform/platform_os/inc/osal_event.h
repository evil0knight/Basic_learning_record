#ifndef OSAL_EVENT_H
#define OSAL_EVENT_H

#include <stdbool.h>
#include "osal_error.h"
#include "osal_types.h"

/*
 * 事件组（event group）抽象
 * 用于多个任务/ISR 之间的一组二进制事件标志的同步与通知。
 * 对应 FreeRTOS 的 EventGroup。
 */

/* 创建事件组 */
osal_status_t osal_event_group_create(osal_event_handle_t *event);

/* 删除事件组 */
osal_status_t osal_event_group_delete(osal_event_handle_t event);

/* 置位一组事件位（任务上下文或 ISR） */
osal_status_t osal_event_group_set_bits(osal_event_handle_t event, uint32_t bits);

/* 清除一组事件位 */
osal_status_t osal_event_group_clear_bits(osal_event_handle_t event, uint32_t bits);

/*
 * 等待指定的事件位。
 * @param bits          等待的位掩码
 * @param clear_on_exit 退出时是否清除已满足的位
 * @param wait_all_bits true=等待全部位，false=等待任一位
 * @param timeout       等待超时（OSAL_WAIT_FOREVER 表示永久等待）
 * @param actual_bits   输出：实际满足的位（可为 NULL）
 */
osal_status_t osal_event_group_wait_bits(osal_event_handle_t event,
                                         uint32_t bits,
                                         bool clear_on_exit,
                                         bool wait_all_bits,
                                         osal_tick_t timeout,
                                         uint32_t *actual_bits);

#endif /* OSAL_EVENT_H */
