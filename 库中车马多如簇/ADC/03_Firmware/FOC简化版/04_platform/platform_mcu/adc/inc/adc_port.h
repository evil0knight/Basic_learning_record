/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file adc_port.h
 *
 * @par dependencies
 * - stdint.h
 * - main.h
 * - adc.h
 *
 * @author YA
 *
 * @brief 声明 ADC 平台接口、实例配置类型及回调类型。
 *
 * Processing flow:
 *
 * 上层调用 core_adc_*，通过注册回调接收 Platform 分发的事件。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef ADC_PORT_H
#define ADC_PORT_H

//****************************** Includes begin ******************************//
#include <stdint.h>

#include "main.h"
#include "adc.h"
//******************************* Includes end *******************************//

//******************************* Enums begin ********************************//
/** @brief ADC 平台操作返回状态。 */
typedef enum
{
    CORE_ADC_OK = 0,
    CORE_ADC_ERROR,
    CORE_ADC_BUSY
} en_core_adc_status_t;

/** @brief ADC 逻辑实例编号，CORE_ADC_MAX 为实例数量和无效值。 */
typedef enum
{
    CORE_ADC1 = 0,
    CORE_ADC2,
    CORE_ADC3,
    CORE_ADC_MAX
} en_core_adc_instance_t;
//******************************** Enums end *********************************//

//****************************** Structs begin *******************************//
/** @brief 单个 ADC 实例的 HAL 句柄和轮询超时配置。 */
typedef struct
{
    ADC_HandleTypeDef *handle;
    uint32_t timeout;
    uint8_t injected_enabled;
    uint8_t injected_count;
} st_adc_config_t;
//******************************* Structs end ********************************//

//****************************** Typedefs begin ******************************//
/**
 * @brief 转换完成通知，参数依次为实例、缓冲区和记录的采样点数。
 * @note 在中断上下文执行；DMA 回传缓冲区和长度，IT 回传 NULL 和 0。
 */
typedef void (*adc_conv_callback_t)(en_core_adc_instance_t instance,
                                    uint16_t *p_buffer,
                                    uint16_t count);

/** @brief Injected 组转换完成通知。*/
typedef void (*adc_injected_callback_t)(en_core_adc_instance_t instance);

/** @brief ADC 错误通知，参数为实例和 HAL 错误码。 */
typedef void (*adc_error_callback_t)(en_core_adc_instance_t instance,
                                     uint32_t error_code);
//******************************* Typedefs end *******************************//

//****************************** Defines begin *******************************//
#define CORE_ADC_DEFAULT_TIMEOUT 10U
//******************************* Defines end ********************************//

//************************ Function Prototypes begin *************************//
/** @brief 初始化 ADC 平台软件状态。*/
en_core_adc_status_t core_adc_init(void);

/** @brief 校准指定 ADC 的单端输入内部偏移 */
en_core_adc_status_t core_adc_calibrate(en_core_adc_instance_t instance);

/** @brief 阻塞读取一次规则组 ADC 转换结果。*/
en_core_adc_status_t core_adc_receive(en_core_adc_instance_t instance,
                                      uint16_t *p_value);

/** @brief 启动指定 ADC 的普通 DMA 采样。*/
en_core_adc_status_t core_adc_receive_dma(
    en_core_adc_instance_t instance,
    uint16_t *p_buffer,
    uint16_t count);

/** @brief 停止指定 ADC 的普通 DMA 采样。*/
en_core_adc_status_t core_adc_stop_dma(en_core_adc_instance_t instance);

/** @brief 启动指定 ADC 的规则组中断采样。*/
en_core_adc_status_t core_adc_receive_it(en_core_adc_instance_t instance);

/** @brief 停止指定 ADC 的规则组中断采样。*/
en_core_adc_status_t core_adc_stop_it(en_core_adc_instance_t instance);

en_core_adc_status_t core_adc_injected_start(
    en_core_adc_instance_t instance);

en_core_adc_status_t core_adc_injected_start_it(
    en_core_adc_instance_t instance);

/** @brief 停止指定 ADC 的 Injected 组中断采样。*/
en_core_adc_status_t core_adc_injected_stop_it(
    en_core_adc_instance_t instance);

/** @brief 读取规则组 ADC 当前转换结果。*/
en_core_adc_status_t core_adc_get_value(
    en_core_adc_instance_t instance,
    uint16_t *p_value);

/** @brief 读取 Injected 组指定 rank 的转换结果。*/
en_core_adc_status_t core_adc_get_injected_value(
    en_core_adc_instance_t instance,
    uint32_t rank,
    uint16_t *p_value);

/** @brief 注册普通规则组/DMA 转换完成回调。*/
en_core_adc_status_t core_adc_register_conv_callback(
    en_core_adc_instance_t instance, adc_conv_callback_t callback);

/** @brief 注册 Injected 组转换完成回调。*/
en_core_adc_status_t core_adc_register_injected_callback(
    en_core_adc_instance_t instance,
    adc_injected_callback_t callback);

/** @brief 注册 ADC 错误回调。*/
en_core_adc_status_t core_adc_register_error_callback(
    en_core_adc_instance_t instance, adc_error_callback_t callback);

/** @brief 获取逻辑 ADC 实例对应的 HAL 句柄。*/
ADC_HandleTypeDef *core_adc_get_handle(en_core_adc_instance_t instance);

//**************************** ISR Interfaces begin **************************//
/** @brief 分发普通规则组转换完成事件。*/
en_core_adc_status_t core_adc_irq_conv_complete(ADC_HandleTypeDef *hadc);

/** @brief 分发 Injected 组转换完成事件。*/
en_core_adc_status_t core_adc_irq_injected_complete(
    ADC_HandleTypeDef *hadc);

en_core_adc_status_t core_adc_irq_error(ADC_HandleTypeDef *hadc);

//***************************** ISR Interfaces end ***************************//

//************************* Function Prototypes end **************************//

#endif
//******************************* End of File ********************************//
