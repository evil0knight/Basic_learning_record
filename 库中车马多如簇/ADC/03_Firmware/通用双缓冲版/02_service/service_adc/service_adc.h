/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file service_adc.h
 *
 * @par dependencies
 * - stdint.h
 * - osal.h
 * - adc_port.h
 *
 * @author YA
 *
 * @brief 声明 ADC 双缓冲采集服务和任务入口。
 *
 * Processing flow:
 *
 * 资源就绪 -> 服务初始化 -> 注册回调并启动 -> 任务处理数据。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef SERVICE_ADC_H
#define SERVICE_ADC_H

//****************************** Includes begin ******************************//
#include <stdint.h>

#include "osal.h"
#include "adc_port.h"
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
/* 每块采样点数，Service 共持有两块等长缓冲区。 */
#define SERVICE_ADC_BUFFER_SIZE 8U
//******************************* Defines end ********************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 初始化 ADC 采集服务及双缓冲状态。
 *
 * Steps:
 *  1. 从统一资源表取得两个任务句柄和互斥量。
 *  2. 检查资源并重置双缓冲写入下标。
 *
 * @return osal_status_t : 成功返回 OSAL_SUCCESS，否则返回资源或初始化错误。
 * @note 由 App 先调用 core_adc_init，再在任务创建后初始化本服务。
 *
 */
osal_status_t service_adc_init(void);

/**
 * @brief 注册采集完成回调并启动首轮采集。
 *
 * Steps:
 *  1. 将服务的通知回调注册到 CORE_ADC1。
 *  2. 将当前写入块交给 ADC 平台启动采集。
 *
 * @return osal_status_t : 启动成功返回 OSAL_SUCCESS，否则返回 OSAL_ERROR。
 * @note 先调用 service_adc_init；本函数每次调用都会设置完成回调。
 *
 */
osal_status_t service_adc_start(void);

/**
 * @brief 请求停止 CORE_ADC1 的采集。
 *
 * Steps:
 *  1. 调用 ADC 平台停止接口。
 *
 * @return void : 当前接口不向调用者返回停止结果。
 * @note 只停止底层采集，不退出任务或清理待处理通知。
 *
 */
void service_adc_stop(void);

/**
 * @brief 等待采集完成、切换写入缓冲区并启动下一轮。
 *
 * Steps:
 *  1. 等待 ADC_BIT_DONE，通知采样任务消费已完成块。
 *  2. 切换写入块，按通知位和互斥量等待该块处理完成。
 *  3. 将新写入块交给 ADC 平台，开始下一轮采集。
 *
 * @param[in] argument : 任务入口参数，当前未使用。
 * @return void : 任务入口持续运行，不正常返回。
 * @note 普通 DMA 完成后由任务重启，两轮采集之间存在间隙。
 *
 */
void service_adc_transfer_task(void *argument);

/**
 * @brief 等待缓冲区就绪并执行采样数据处理。
 *
 * Steps:
 *  1. 等待产生通知，依次检查两块缓冲区。
 *  2. 取得互斥量，发送消费通知并调用业务处理入口。
 *  3. 清除本块产生位并释放互斥量。
 *
 * @param[in] argument : 任务入口参数，当前未使用。
 * @return void : 任务入口持续运行，不正常返回。
 * @note 业务处理在任务上下文执行，处理期间持有 ADC 互斥量。
 *
 */
void service_adc_sample_task(void *argument);

/**
 * @brief 处理一块已完成采集的数据。
 *
 * Steps:
 *  1. 由采样任务调用，默认弱实现不处理数据。
 *
 * @param[in] p_buffer : 已完成缓冲区地址，处理期间由采样任务持锁。
 * @param[in] count : 本块采样点数。
 * @return void : 业务处理入口无返回值。
 * @note 工程可提供同名强定义完成滤波、换算或上报；返回后不再占用缓冲区。
 *
 */
void service_adc_user_process(uint16_t *p_buffer, uint16_t count);

//************************* Function Prototypes end **************************//

#endif
//******************************* End of File ********************************//
