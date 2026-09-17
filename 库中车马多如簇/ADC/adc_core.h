#ifndef ADC_CORE_H
#define ADC_CORE_H

#include <stdint.h>

#include "osal.h"
#include "adc_port.h"

/*
 * ADC 双缓冲与任务层
 *
 * 两个任务和一个互斥量从统一资源表取（user_task_get /
 * user_mutex_get）。事件用任务通知传递，通知位在 adc_core.c 里定义：
 * 中断 → 搬运任务（完成位），搬运任务 → 采样任务（产生位），
 * 采样任务 → 搬运任务（消费位）。单块缓冲区采样点数由
 * ADC_CORE_BUFFER_SIZE 决定。
 *
 * 任务句柄是通知的目标，所以 adc_core_init() 必须放在
 * user_tasks_start() 之后调用。
 */

/* 单块缓冲区采样点数：双缓冲共 2 块 */
#define ADC_CORE_BUFFER_SIZE 8U

/* 初始化：取任务句柄和互斥量，保留句柄 */
osal_status_t adc_core_init(void);

/* 注册完成回调并启动第一轮搬运 */
osal_status_t adc_core_start(void);

/* 停止搬运 */
void adc_core_stop(void);

/* 搬运任务：等完成通知、切换双缓冲、启动下一轮 */
void adc_transfer_task(void *argument);

/* 采样任务：等产生通知、取互斥量、处理已完成缓冲区 */
void adc_sample_task(void *argument);

/* 业务处理入口：工程自己实现，默认空实现 */
void adc_user_process(uint16_t *p_buffer, uint16_t count);

#endif /* ADC_CORE_H */
