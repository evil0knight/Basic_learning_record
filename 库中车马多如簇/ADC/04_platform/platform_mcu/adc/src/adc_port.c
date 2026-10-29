/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file adc_port.c
 *
 * @par dependencies
 * - adc_port.h
 * - adc_port_config.h
 *
 * @author YA
 *
 * @brief ADC 平台封装及转换完成、错误事件分发。
 *
 * Processing flow:
 *
 * 参数检查 -> HAL 采集接口 -> 返回统一状态；App 事件 -> 已注册回调。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "adc_port.h"

#include "adc_port_config.h"
//******************************* Includes end *******************************//

//************************** Static Variables begin **************************//
static adc_conv_callback_t g_adc_conv_callbacks[CORE_ADC_MAX] = {NULL};
static adc_error_callback_t g_adc_error_callbacks[CORE_ADC_MAX] = {NULL};

/* 记录最近一次启动接收时提供的缓冲区和采样长度。 */
static uint16_t *g_adc_buffers[CORE_ADC_MAX] = {NULL};
static uint16_t g_adc_counts[CORE_ADC_MAX] = {0U};
//*************************** Static Variables end ***************************//

//************************** Static Functions begin **************************//
/**
 * @brief 转换 HAL 返回值为 ADC 平台状态。
 *
 * Steps:
 *  1. 按 HAL 状态映射成功、忙或错误结果。
 *
 * @param[in] hal_status : HAL 操作返回状态。
 * @return en_core_adc_status_t : 统一的 ADC 操作状态。
 *
 */
static en_core_adc_status_t core_adc_from_hal(
    HAL_StatusTypeDef hal_status)
{
    switch (hal_status)
    {
        case HAL_OK:
            return CORE_ADC_OK;
        case HAL_BUSY:
            return CORE_ADC_BUSY;
        case HAL_ERROR:
        case HAL_TIMEOUT:
        default:
            return CORE_ADC_ERROR;
    }
}

/**
 * @brief 根据 HAL ADC 句柄查找逻辑实例。
 *
 * Steps:
 *  1. 检查句柄，再遍历实例配置表。
 *
 * @param[in] hadc : HAL ADC 句柄。
 * @return en_core_adc_instance_t : 匹配的实例，未找到时返回 CORE_ADC_MAX。
 *
 */
static en_core_adc_instance_t core_adc_instance_from_handle(
    ADC_HandleTypeDef *hadc)
{
    uint32_t i;

    if (hadc == NULL)
    {
        return CORE_ADC_MAX;
    }

    for (i = 0U; i < CORE_ADC_MAX; i++)
    {
        if (g_adc_configs[i].handle == hadc)
        {
            return (en_core_adc_instance_t)i;
        }
    }

    return CORE_ADC_MAX;
}

/**
 * @brief 检查 ADC 实例、接收缓冲区和采样长度。
 *
 * Steps:
 *  1. 检查实例范围、缓冲区、长度及已配置的 HAL 句柄。
 *
 * @param[in] instance : 逻辑 ADC 实例。
 * @param[in] p_buffer : 采样缓冲区地址。
 * @param[in] count : 本轮采样点数。
 * @return en_core_adc_status_t : 参数有效返回 CORE_ADC_OK，否则返回错误。
 *
 */
static en_core_adc_status_t core_adc_validate(
    en_core_adc_instance_t instance,
    const uint16_t *p_buffer,
    uint16_t count)
{
    if (instance >= CORE_ADC_MAX)
    {
        return CORE_ADC_ERROR;
    }
    if ((p_buffer == NULL) || (count == 0U))
    {
        return CORE_ADC_ERROR;
    }
    if (g_adc_configs[instance].handle == NULL)
    {
        return CORE_ADC_ERROR;
    }

    return CORE_ADC_OK;
}

//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
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
en_core_adc_status_t core_adc_init(void)
{
    uint32_t i;

    for (i = 0U; i < CORE_ADC_MAX; i++)
    {
        g_adc_conv_callbacks[i] = NULL;
        g_adc_error_callbacks[i] = NULL;
        g_adc_buffers[i] = NULL;
        g_adc_counts[i] = 0U;
    }

    return CORE_ADC_OK;
}

/**
 * @brief 以堵塞方式读取一次 ADC 转换结果。
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
                                      uint16_t *p_value)
{
    ADC_HandleTypeDef *p_handle;

    if ((instance >= CORE_ADC_MAX) || (p_value == NULL))
    {
        return CORE_ADC_ERROR;
    }

    p_handle = g_adc_configs[instance].handle;
    if (p_handle == NULL)
    {
        return CORE_ADC_ERROR;
    }

    if (HAL_ADC_Start(p_handle) != HAL_OK)//开启一次转换,adc独有操作
    {
        return CORE_ADC_ERROR;
    }

    if (HAL_ADC_PollForConversion(p_handle,//堵塞检查完成标志
                                  g_adc_configs[instance].timeout) != HAL_OK)
    {
        (void)HAL_ADC_Stop(p_handle);
        return CORE_ADC_ERROR;
    }

    *p_value = (uint16_t)HAL_ADC_GetValue(p_handle);
    (void)HAL_ADC_Stop(p_handle);

    return CORE_ADC_OK;
}

/**
 * @brief 以 DMA 方式启动指定长度的 ADC 采集。
 *        完成会触发回调
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
    uint16_t count)
{
    en_core_adc_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_adc_validate(instance, p_buffer, count);
    if (CORE_ADC_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    g_adc_buffers[instance] = p_buffer;
    g_adc_counts[instance] = count;

    return core_adc_from_hal(HAL_ADC_Start_DMA(
        g_adc_configs[instance].handle, (uint32_t *)p_buffer, count));
}

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
en_core_adc_status_t core_adc_stop_dma(en_core_adc_instance_t instance)
{
    //-------------------------参数检查 begin-------------------------//
    if (((uint32_t)instance >= (uint32_t)CORE_ADC_MAX) ||
        (NULL == g_adc_configs[instance].handle))
    {
        return CORE_ADC_ERROR;
    }
    //--------------------------参数检查 end--------------------------//

    return core_adc_from_hal(
        HAL_ADC_Stop_DMA(g_adc_configs[instance].handle));
}

/**
 * @brief 启动 ADC 转换完成中断，不使用 DMA 搬运。
 *        中断搬运,搬运完成后中断回调
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
en_core_adc_status_t core_adc_receive_it(en_core_adc_instance_t instance)
{
    //-------------------------参数检查 begin-------------------------//
    if (((uint32_t)instance >= (uint32_t)CORE_ADC_MAX) ||
        (NULL == g_adc_configs[instance].handle))
    {
        return CORE_ADC_ERROR;
    }
    //--------------------------参数检查 end--------------------------//

    g_adc_buffers[instance] = NULL;
    g_adc_counts[instance] = 0U;

    return core_adc_from_hal(HAL_ADC_Start_IT(g_adc_configs[instance].handle));
}

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
en_core_adc_status_t core_adc_stop_it(en_core_adc_instance_t instance)
{
    //-------------------------参数检查 begin-------------------------//
    if (((uint32_t)instance >= (uint32_t)CORE_ADC_MAX) ||
        (NULL == g_adc_configs[instance].handle))
    {
        return CORE_ADC_ERROR;
    }
    //--------------------------参数检查 end--------------------------//

    return core_adc_from_hal(HAL_ADC_Stop_IT(g_adc_configs[instance].handle));
}

/**
 * @brief 立刻读取ADC数据寄存器,转换完成后调用
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
    uint16_t *p_value)
{
    //-------------------------参数检查 begin-------------------------//
    if (((uint32_t)instance >= (uint32_t)CORE_ADC_MAX) ||
        (NULL == p_value) || (NULL == g_adc_configs[instance].handle))
    {
        return CORE_ADC_ERROR;
    }
    //--------------------------参数检查 end--------------------------//

    *p_value = (uint16_t)HAL_ADC_GetValue(g_adc_configs[instance].handle);
    return CORE_ADC_OK;
}

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
    en_core_adc_instance_t instance, adc_conv_callback_t callback)
{
    if (instance >= CORE_ADC_MAX)
    {
        return CORE_ADC_ERROR;
    }

    g_adc_conv_callbacks[instance] = callback;
    return CORE_ADC_OK;
}

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
    en_core_adc_instance_t instance, adc_error_callback_t callback)
{
    if (instance >= CORE_ADC_MAX)
    {
        return CORE_ADC_ERROR;
    }

    g_adc_error_callbacks[instance] = callback;
    return CORE_ADC_OK;
}

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
ADC_HandleTypeDef *core_adc_get_handle(en_core_adc_instance_t instance)
{
    return (instance < CORE_ADC_MAX) ? g_adc_configs[instance].handle
                                     : NULL;
}

/**
 * @brief 分发 App 转发的 ADC 转换完成事件。
 *        app层的isr_handler里的回调入口调用
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
en_core_adc_status_t core_adc_irq_conv_complete(ADC_HandleTypeDef *hadc)
{
    en_core_adc_instance_t instance;
    if (NULL == hadc)
    {
        return CORE_ADC_ERROR;
    }

    instance = core_adc_instance_from_handle(hadc);
    if (instance >= CORE_ADC_MAX)
    {
        return CORE_ADC_ERROR;
    }

    if ((instance < CORE_ADC_MAX) &&
        (g_adc_conv_callbacks[instance] != NULL))
    {
        g_adc_conv_callbacks[instance](instance,
                                       g_adc_buffers[instance],
                                       g_adc_counts[instance]);
    }
    return CORE_ADC_OK;
}

/**
 * @brief 分发 App 转发的 ADC 错误事件。
 *        app层的isr_handler里的回调入口调用
 * Steps:
 *  1. 检查句柄并查找逻辑实例。
 *  2. 将 HAL 错误码传给已注册的错误回调。
 *
 * @param[in] hadc : 发生错误的 HAL ADC 句柄。
 * @return en_core_adc_status_t : 句柄有效返回 CORE_ADC_OK，否则返回错误。
 * @note 仅供 App HAL 回调调用。
 *
 */
en_core_adc_status_t core_adc_irq_error(ADC_HandleTypeDef *hadc)
{
    en_core_adc_instance_t instance;
    if (NULL == hadc)
    {
        return CORE_ADC_ERROR;
    }

    instance = core_adc_instance_from_handle(hadc);
    if (instance >= CORE_ADC_MAX)
    {
        return CORE_ADC_ERROR;
    }

    if ((instance < CORE_ADC_MAX) && (hadc != NULL) &&
        (g_adc_error_callbacks[instance] != NULL))
    {
        g_adc_error_callbacks[instance](instance, hadc->ErrorCode);
    }
    return CORE_ADC_OK;
}

//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
