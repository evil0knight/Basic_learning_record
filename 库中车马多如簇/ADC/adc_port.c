#include "adc_port.h"

#include "adc_port_config.h"

/*
 * ADC 平台抽象层实现
 *
 * core_adc_* 统一接口，内部转 HAL。DMA 与转换完成中断只在这里切换，
 * 回调桥先用句柄反查逻辑实例，再调用上层注册的函数指针。
 */

static adc_conv_callback_t g_adc_conv_callbacks[CORE_ADC_MAX] = {NULL};
static adc_error_callback_t g_adc_error_callbacks[CORE_ADC_MAX] = {NULL};

/* 最近一次 core_adc_start 的缓冲，回调时回传给上层 */
static uint16_t *g_adc_buffers[CORE_ADC_MAX] = {NULL};
static uint16_t g_adc_counts[CORE_ADC_MAX] = {0U};

/* 将 STM32 HAL 返回值转换为上层统一的 ADC 状态码。 */
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

/* 根据 ADC 句柄查找对应的逻辑实例；未找到时返回无效实例。 */
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

/* 校验实例、缓冲区、长度和句柄，避免把非法参数传给 HAL。 */
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

/* 初始化：清回调和缓冲记录，句柄由 CubeMX 的 MX_ADCx_Init 初始化 */
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

/* 轮询单次测量：启动、等转换完成、读值、停止 */
en_core_adc_status_t core_adc_measure(en_core_adc_instance_t instance,
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

    if (HAL_ADC_Start(p_handle) != HAL_OK)
    {
        return CORE_ADC_ERROR;
    }

    if (HAL_ADC_PollForConversion(p_handle,
                                  g_adc_configs[instance].timeout) != HAL_OK)
    {
        (void)HAL_ADC_Stop(p_handle);
        return CORE_ADC_ERROR;
    }

    *p_value = (uint16_t)HAL_ADC_GetValue(p_handle);
    (void)HAL_ADC_Stop(p_handle);

    return CORE_ADC_OK;
}

/* 启动单次搬运：DMA 模式走 DMA，否则走转换完成中断 */
en_core_adc_status_t core_adc_start(en_core_adc_instance_t instance,
                                    uint16_t *p_buffer,
                                    uint16_t count)
{
    ADC_HandleTypeDef *p_handle;
    en_core_adc_status_t status;

    status = core_adc_validate(instance, p_buffer, count);
    if (status != CORE_ADC_OK)
    {
        return status;
    }

    p_handle = g_adc_configs[instance].handle;
    g_adc_buffers[instance] = p_buffer;
    g_adc_counts[instance] = count;

#if ADC_PORT_USE_DMA
    return core_adc_from_hal(
        HAL_ADC_Start_DMA(p_handle, (uint32_t *)p_buffer, (uint32_t)count));
#else
    return core_adc_from_hal(HAL_ADC_Start_IT(p_handle));
#endif
}

/* 停止搬运 */
en_core_adc_status_t core_adc_stop(en_core_adc_instance_t instance)
{
    if (instance >= CORE_ADC_MAX)
    {
        return CORE_ADC_ERROR;
    }

    if (g_adc_configs[instance].handle == NULL)
    {
        return CORE_ADC_ERROR;
    }

#if ADC_PORT_USE_DMA
    return core_adc_from_hal(
        HAL_ADC_Stop_DMA(g_adc_configs[instance].handle));
#else
    return core_adc_from_hal(
        HAL_ADC_Stop_IT(g_adc_configs[instance].handle));
#endif
}

/* 注册转换完成回调 */
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

/* 注册错误回调 */
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

/* 根据逻辑实例取 HAL 句柄（特殊场景直接操作 HAL 用） */
ADC_HandleTypeDef *core_adc_get_handle(en_core_adc_instance_t instance)
{
    return (instance < CORE_ADC_MAX) ? g_adc_configs[instance].handle
                                     : NULL;
}

/* ===== HAL 回调桥 =====
   DMA 模式下由 DMA 流中断调 HAL_DMA_IRQHandler 触发，
   非 DMA 模式下由 ADC 中断调 HAL_ADC_IRQHandler 触发，
   两条路径都进这里，再调上层注册的处理函数。 */

/**
 * @brief ADC 转换完成回调
 *
 * @param hadc 触发回调的 ADC HAL 句柄
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    en_core_adc_instance_t instance = core_adc_instance_from_handle(hadc);

    if ((instance < CORE_ADC_MAX) &&
        (g_adc_conv_callbacks[instance] != NULL))
    {
        g_adc_conv_callbacks[instance](instance,
                                       g_adc_buffers[instance],
                                       g_adc_counts[instance]);
    }
}

/**
 * @brief ADC 错误回调
 *
 * @param hadc 发生错误的 ADC HAL 句柄
 */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    en_core_adc_instance_t instance = core_adc_instance_from_handle(hadc);

    if ((instance < CORE_ADC_MAX) && (hadc != NULL) &&
        (g_adc_error_callbacks[instance] != NULL))
    {
        g_adc_error_callbacks[instance](instance, hadc->ErrorCode);
    }
}
