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
    CORE_ADC_MAX
} en_core_adc_instance_t;
//******************************** Enums end *********************************//

//****************************** Structs begin *******************************//
/** @brief 单个 ADC 实例的 HAL 句柄和轮询超时配置。 */
typedef struct
{
    ADC_HandleTypeDef *handle;
    uint32_t timeout;
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

/** @brief ADC 错误通知，参数为实例和 HAL 错误码。 */
typedef void (*adc_error_callback_t)(en_core_adc_instance_t instance,
                                     uint32_t error_code);
//******************************* Typedefs end *******************************//

//****************************** Defines begin *******************************//
#define CORE_ADC_DEFAULT_TIMEOUT 10U
//******************************* Defines end ********************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 初始化 ADC 平台的软件状态。
 *
 * Steps:
 *  1. 清空回调、缓冲区地址和采样长度记录。
 *
 * @return en_core_adc_status_t : 返回 CORE_ADC_OK。
 * @note 硬件由 CubeMX 启动流程初始化；本函数应先于回调注册和采集执行。
 *
 */
en_core_adc_status_t core_adc_init(void);

/**
 * @brief 以轮询方式读取一次 ADC 转换结果。
 *
 * Steps:
 *  1. 检查参数并启动 ADC。
 *  2. 等待转换完成，读取结果后停止 ADC。
 *
 * @param[in] instance : 逻辑 ADC 实例。
 * @param[out] p_value : 接收转换结果的变量地址。
 * @return en_core_adc_status_t : 成功返回 CORE_ADC_OK，否则返回错误。
 * @note 调用会等待转换完成或超时。
 *
 */
en_core_adc_status_t core_adc_receive(en_core_adc_instance_t instance,
                                      uint16_t *p_value);

/**
 * @brief 以 DMA 方式启动指定长度的 ADC 采集。
 *
 * Steps:
 *  1. 检查实例、缓冲区和采样长度。
 *  2. 保存缓冲区信息并调用 HAL_ADC_Start_DMA。
 *
 * @param[in] instance : 逻辑 ADC 实例。
 * @param[out] p_buffer : DMA 采样缓冲区，传输期间必须保持有效。
 * @param[in] count : 本次 DMA 采样点数。
 * @return en_core_adc_status_t : HAL 启动结果对应的平台状态。
 * @note DMA 数据宽度须与 uint16_t 缓冲区匹配；Normal 模式完成后再启动。
 *
 */
en_core_adc_status_t core_adc_receive_dma(
    en_core_adc_instance_t instance,
    uint16_t *p_buffer,
    uint16_t count);

/**
 * @brief 停止指定 ADC 的 DMA 采集。
 *
 * Steps:
 *  1. 检查实例和 HAL 句柄，再调用 HAL_ADC_Stop_DMA。
 *
 * @param[in] instance : 逻辑 ADC 实例。
 * @return en_core_adc_status_t : HAL 停止结果对应的平台状态。
 *
 */
en_core_adc_status_t core_adc_stop_dma(en_core_adc_instance_t instance);

/**
 * @brief 启动 ADC 转换完成中断，不使用 DMA 搬运。
 *
 * Steps:
 *  1. 检查实例和句柄，清空本实例的 DMA 缓冲区记录。
 *  2. 调用 HAL_ADC_Start_IT，转换完成后由回调通知使用者。
 *
 * @param[in] instance : 逻辑 ADC 实例。
 * @return en_core_adc_status_t : HAL 启动结果对应的平台状态。
 * @note 回调的缓冲区为 NULL、长度为 0；在回调中调用 core_adc_get_value
 *       读取转换结果，本接口不累计填充软件缓冲区。
 *
 */
en_core_adc_status_t core_adc_receive_it(en_core_adc_instance_t instance);

/**
 * @brief 停止指定 ADC 的中断采集。
 *
 * Steps:
 *  1. 检查实例和句柄，再调用 HAL_ADC_Stop_IT。
 *
 * @param[in] instance : 逻辑 ADC 实例。
 * @return en_core_adc_status_t : HAL 停止结果对应的平台状态。
 *
 */
en_core_adc_status_t core_adc_stop_it(en_core_adc_instance_t instance);

/**
 * @brief 由 CPU 读取 ADC 当前转换结果，不启动或等待转换。
 *
 * Steps:
 *  1. 检查实例、输出地址和 HAL 句柄。
 *  2. 调用 HAL_ADC_GetValue，将结果写入输出变量。
 *
 * @param[in] instance : 逻辑 ADC 实例。
 * @param[out] p_value : 转换结果输出地址。
 * @return en_core_adc_status_t : 读取成功返回 CORE_ADC_OK。
 * @note 中断采集时在完成回调中读取；多通道逐点读取需配置对应 EOC 事件。
 *
 */
en_core_adc_status_t core_adc_get_value(
    en_core_adc_instance_t instance,
    uint16_t *p_value);

/**
 * @brief 设置指定 ADC 实例的转换完成回调。
 *
 * Steps:
 *  1. 检查实例范围，再保存回调函数地址。
 *
 * @param[in] instance : 逻辑 ADC 实例。
 * @param[in] callback : 转换完成回调，NULL 表示取消回调。
 * @return en_core_adc_status_t : 注册成功返回 CORE_ADC_OK。
 * @note 重复设置会覆盖原回调；回调在中断上下文执行。
 *
 */
en_core_adc_status_t core_adc_register_conv_callback(
    en_core_adc_instance_t instance, adc_conv_callback_t callback);

/**
 * @brief 设置指定 ADC 实例的错误回调。
 *
 * Steps:
 *  1. 检查实例范围，再保存回调函数地址。
 *
 * @param[in] instance : 逻辑 ADC 实例。
 * @param[in] callback : 错误回调，NULL 表示取消回调。
 * @return en_core_adc_status_t : 注册成功返回 CORE_ADC_OK。
 * @note 重复设置会覆盖原回调；回调在中断上下文执行。
 *
 */
en_core_adc_status_t core_adc_register_error_callback(
    en_core_adc_instance_t instance, adc_error_callback_t callback);

/**
 * @brief 获取逻辑 ADC 实例对应的 HAL 句柄。
 *
 * Steps:
 *  1. 检查实例范围，返回配置表中的句柄。
 *
 * @param[in] instance : 逻辑 ADC 实例。
 * @return ADC_HandleTypeDef * : 已配置句柄，实例非法或未配置时为 NULL。
 *
 */
ADC_HandleTypeDef *core_adc_get_handle(en_core_adc_instance_t instance);

/**
 * @brief 分发 App 转发的 ADC 转换完成事件。
 *
 * Steps:
 *  1. 检查句柄并查找逻辑实例。
 *  2. 向已注册回调传递实例、缓冲区地址和记录的采样长度。
 *
 * @param[in] hadc : 触发转换完成事件的 HAL ADC 句柄。
 * @return en_core_adc_status_t : 句柄有效返回 CORE_ADC_OK，否则返回错误。
 * @note 仅供 App HAL 回调调用；未注册回调时不执行通知。
 *
 */
en_core_adc_status_t core_adc_irq_conv_complete(ADC_HandleTypeDef *hadc);

/**
 * @brief 分发 App 转发的 ADC 错误事件。
 *
 * Steps:
 *  1. 检查句柄并查找逻辑实例。
 *  2. 将 HAL 错误码传给已注册的错误回调。
 *
 * @param[in] hadc : 发生错误的 HAL ADC 句柄。
 * @return en_core_adc_status_t : 句柄有效返回 CORE_ADC_OK，否则返回错误。
 * @note 仅供 App HAL 回调调用。
 *
 */
en_core_adc_status_t core_adc_irq_error(ADC_HandleTypeDef *hadc);

//************************* Function Prototypes end **************************//

#endif
//******************************* End of File ********************************//
