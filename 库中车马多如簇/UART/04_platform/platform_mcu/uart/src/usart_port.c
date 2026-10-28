/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file usart_port.c
 *
 * @par dependencies
 * - usart_port.h
 * - usart_port_config.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief USART 平台抽象层实现，负责 HAL 调用、DMA 与回调桥接。
 *
 * Processing flow:
 *
 * 参数检查 -> 调用 HAL -> 转换状态码 -> 返回结果。
 *
 * @version V1.0 2026-10-27
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "usart_port.h"

#include "usart_port_config.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//************************** Static Variables begin **************************//
static usart_rx_callback_t g_pf_usart_rx_callbacks[CORE_USART_MAX] =
{
    NULL
};
static usart_tx_callback_t g_pf_usart_tx_callbacks[CORE_USART_MAX] =
{
    NULL
};
static usart_error_callback_t g_pf_usart_error_callbacks[CORE_USART_MAX] =
{
    NULL
};

static uint8_t *g_p_usart_rx_buffer[CORE_USART_MAX] =
{
    NULL
};
static volatile uint16_t g_usart_dma_rx_size[CORE_USART_MAX] =
{
    0U
};
static volatile uint8_t g_usart_dma_rx_done[CORE_USART_MAX] =
{
    0U
};
//*************************** Static Variables end ***************************//

//************************** Static Functions begin **************************//
/**
 * @brief DMA 同步接收的内部回调，记录实际接收长度并置完成标志。
 *        synchronous:同步
 * @param[in] p_data 接收缓冲区地址。
 * @param[in] size 本次实际接收字节数。
 */
static void core_usart_dma_sync_callback(uint8_t *p_data, uint16_t size)
{
    uint32_t i;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_data)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    for (i = 0U; i < (uint32_t)CORE_USART_MAX; i++)
    {
        if (g_p_usart_rx_buffer[i] == p_data)
        {
            g_usart_dma_rx_size[i] = size;
            g_usart_dma_rx_done[i] = 1U;
            break;
        }
    }
}

/**
 * @brief 将 STM32 HAL 返回值转换为统一 USART 状态码。
 *
 * @param[in] hal_status HAL 返回状态。
 * @return 统一 USART 状态码。
 */
static en_core_usart_status_t core_usart_from_hal(
    HAL_StatusTypeDef hal_status)
{
    en_core_usart_status_t status;

    switch (hal_status)
    {
        case HAL_OK:
            status = CORE_USART_OK;
            break;
        case HAL_BUSY:
            status = CORE_USART_BUSY;
            break;
        case HAL_TIMEOUT:
            status = CORE_USART_TIMEOUT;
            break;
        case HAL_ERROR:
        default:
            status = CORE_USART_ERROR;
            break;
    }

    return status;
}

/**
 * @brief 校验逻辑串口实例编号是否合法。
 *
 * @param[in] instance 逻辑串口实例。
 * @return USART 操作状态。
 */
static en_core_usart_status_t core_usart_validate_instance(
    en_core_usart_instance_t instance)
{
    //-------------------------参数检查 begin-------------------------//
    if ((uint32_t)instance >= (uint32_t)CORE_USART_MAX)
    {
        return CORE_USART_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return CORE_USART_OK;
}

/**
 * @brief 校验实例、数据缓冲区及串口句柄是否可用。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[in] p_data 数据缓冲区。
 * @param[in] size 数据长度。
 * @return USART 操作状态。
 */
static en_core_usart_status_t core_usart_validate(
    en_core_usart_instance_t instance,
    const void *p_data,
    uint16_t size)
{
    en_core_usart_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_usart_validate_instance(instance);
    if (CORE_USART_OK != status)
    {
        return status;
    }

    if ((NULL == p_data) || (0U == size))
    {
        return CORE_USART_ERR_ARGUMENT;
    }

    if (NULL == g_usart_configs[instance].p_handle)
    {
        return CORE_USART_ERR_NOT_CONFIGURED;
    }
    //--------------------------参数检查 end--------------------------//

    return CORE_USART_OK;
}

/**
 * @brief 根据 HAL 句柄查找对应的逻辑串口实例。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return 逻辑串口实例；未找到时返回 CORE_USART_MAX。
 */
static en_core_usart_instance_t core_usart_instance_from_handle(
    UART_HandleTypeDef *p_handle)
{
    uint32_t i;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return CORE_USART_MAX;
    }
    //--------------------------参数检查 end--------------------------//

    for (i = 0U; i < (uint32_t)CORE_USART_MAX; i++)
    {
        if (g_usart_configs[i].p_handle == p_handle)
        {
            return (en_core_usart_instance_t)i;
        }
    }

    return CORE_USART_MAX;
}
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 初始化 USART 平台层并清空回调状态。
 *
 * Steps:
 *  1. 调用 CubeMX 生成的 USART 初始化函数。
 *  2. 清空回调、接收缓冲区及 DMA 状态。
 *
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_init(void)
{
    uint32_t i;

    MX_USART1_UART_Init();

    for (i = 0U; i < (uint32_t)CORE_USART_MAX; i++)
    {
        g_pf_usart_rx_callbacks[i] = NULL;
        g_pf_usart_tx_callbacks[i] = NULL;
        g_pf_usart_error_callbacks[i] = NULL;
        g_p_usart_rx_buffer[i] = NULL;
        g_usart_dma_rx_size[i] = 0U;
        g_usart_dma_rx_done[i] = 0U;
    }

    return CORE_USART_OK;
}

/**
 * @brief 以阻塞方式发送数据。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[in] p_data 发送缓冲区。
 * @param[in] size 发送字节数。
 * @param[in] timeout 超时时间，单位 ms。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_transmit(
    en_core_usart_instance_t instance,
    const uint8_t *p_data,
    uint16_t size,
    uint32_t timeout)
{
    en_core_usart_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_usart_validate(instance, p_data, size);
    if (CORE_USART_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    return core_usart_from_hal(
        HAL_UART_Transmit(g_usart_configs[instance].p_handle,
                          p_data,
                          size,
                          timeout));
}

/**
 * @brief 以阻塞方式接收数据。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[out] p_data 接收缓冲区。
 * @param[in] size 接收字节数。
 * @param[in] timeout 超时时间，单位 ms。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_receive(
    en_core_usart_instance_t instance,
    uint8_t *p_data,
    uint16_t size,
    uint32_t timeout)
{
    en_core_usart_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_usart_validate(instance, p_data, size);
    if (CORE_USART_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    return core_usart_from_hal(
        HAL_UART_Receive(g_usart_configs[instance].p_handle,
                         p_data,
                         size,
                         timeout));
}

/**
 * @brief 以 DMA 方式发送数据,发送完缓冲区就回调一次
 *
 * @param[in] instance 逻辑串口实例。
 * @param[in] p_data 发送缓冲区。
 * @param[in] size 发送字节数。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_transmit_dma(
    en_core_usart_instance_t instance,
    const uint8_t *p_data,
    uint16_t size)
{
    en_core_usart_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_usart_validate(instance, p_data, size);
    if (CORE_USART_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    return core_usart_from_hal(
        HAL_UART_Transmit_DMA(g_usart_configs[instance].p_handle,
                              p_data,
                              size));
}

/**
 * @brief 以 DMA 方式启动一轮接收
 *
 * @param[in] instance 逻辑串口实例。
 * @param[out] p_data 接收缓冲区。
 * @param[in] size 接收缓冲区大小。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_receive_dma(
    en_core_usart_instance_t instance,
    uint8_t *p_data,
    uint16_t size)
{
    en_core_usart_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_usart_validate(instance, p_data, size);
    if (CORE_USART_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    return core_usart_from_hal(
        HAL_UART_Receive_DMA(g_usart_configs[instance].p_handle,
                             p_data,
                             size));
}

/**
 * @brief 以 DMA + 空闲中断方式启动一轮接收。
 *
 * 一帧接收完成后通过已注册的 RX 回调通知上层。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[out] p_data 接收缓冲区。
 * @param[in] size 接收缓冲区大小。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_receive_to_idle_dma(
    en_core_usart_instance_t instance,
    uint8_t *p_data,
    uint16_t size)
{
    UART_HandleTypeDef *p_handle;
    HAL_StatusTypeDef hal_status;
    en_core_usart_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_usart_validate(instance, p_data, size);
    if (CORE_USART_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    p_handle = g_usart_configs[instance].p_handle;
    g_p_usart_rx_buffer[instance] = p_data;

    hal_status = HAL_UARTEx_ReceiveToIdle_DMA(p_handle, p_data, size);
    if ((HAL_OK == hal_status) && (NULL != p_handle->hdmarx))
    {
        /* 关闭半传输中断，只保留全传输和空闲中断。 */
        __HAL_DMA_DISABLE_IT(p_handle->hdmarx, DMA_IT_HT);
    }

    return core_usart_from_hal(hal_status);
}

/**
 * @brief 以 DMA + 空闲中断方式同步接收一帧。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[out] p_data 接收缓冲区。
 * @param[in] size 接收缓冲区大小。
 * @param[out] p_received_size 实际接收字节数。
 * @param[in] timeout 超时时间，单位 ms；0 表示一直等待。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_receive_to_idle_dma_sync(
    en_core_usart_instance_t instance,
    uint8_t *p_data,
    uint16_t size,
    uint16_t *p_received_size,
    uint32_t timeout)
{
    uint32_t start_tick;
    en_core_usart_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_received_size)
    {
        return CORE_USART_ERR_ARGUMENT;
    }

    status = core_usart_validate(instance, p_data, size);
    if (CORE_USART_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    g_usart_dma_rx_done[instance] = 0U;
    g_usart_dma_rx_size[instance] = 0U;

    status = core_usart_register_rx_callback(
        instance, core_usart_dma_sync_callback);
    if (CORE_USART_OK != status)
    {
        return status;
    }

    status = core_usart_receive_to_idle_dma(instance, p_data, size);
    if (CORE_USART_OK != status)
    {
        return status;
    }

    start_tick = HAL_GetTick();
    while (0U == g_usart_dma_rx_done[instance])
    {
        if ((0U != timeout) &&
            ((HAL_GetTick() - start_tick) >= timeout))
        {
            return CORE_USART_TIMEOUT;
        }
    }

    *p_received_size = g_usart_dma_rx_size[instance];
    return CORE_USART_OK;
}

/**
 * @brief 注册 RX 完成回调。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[in] pf_callback 回调函数指针。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_register_rx_callback(
    en_core_usart_instance_t instance,
    usart_rx_callback_t pf_callback)
{
    en_core_usart_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_usart_validate_instance(instance);
    if (CORE_USART_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    g_pf_usart_rx_callbacks[instance] = pf_callback;
    return CORE_USART_OK;
}

/**
 * @brief 注册 TX 完成回调。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[in] pf_callback 回调函数指针。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_register_tx_callback(
    en_core_usart_instance_t instance,
    usart_tx_callback_t pf_callback)
{
    en_core_usart_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_usart_validate_instance(instance);
    if (CORE_USART_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    g_pf_usart_tx_callbacks[instance] = pf_callback;
    return CORE_USART_OK;
}

/**
 * @brief 注册错误回调。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[in] pf_callback 回调函数指针。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_register_error_callback(
    en_core_usart_instance_t instance,
    usart_error_callback_t pf_callback)
{
    en_core_usart_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_usart_validate_instance(instance);
    if (CORE_USART_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    g_pf_usart_error_callbacks[instance] = pf_callback;
    return CORE_USART_OK;
}

/**
 * @brief 获取底层 HAL UART 句柄。
 *
 * @param[in] instance 逻辑串口实例。
 * @return HAL 句柄；实例非法时返回 NULL。
 */
UART_HandleTypeDef *core_usart_get_handle(
    en_core_usart_instance_t instance)
{
    //-------------------------参数检查 begin-------------------------//
    if ((uint32_t)instance >= (uint32_t)CORE_USART_MAX)
    {
        return NULL;
    }
    //--------------------------参数检查 end--------------------------//

    return g_usart_configs[instance].p_handle;
}

/**
 * @brief 转发 HAL 接收完成事件到已注册的 RX 回调。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return USART 操作状态。
 * @note 仅供 App ISR Handler 调用。
 */
en_core_usart_status_t core_usart_irq_rx_complete(
    UART_HandleTypeDef *p_handle)
{
    en_core_usart_instance_t instance;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return CORE_USART_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    instance = core_usart_instance_from_handle(p_handle);
    if (CORE_USART_MAX == instance)
    {
        return CORE_USART_ERR_NOT_CONFIGURED;
    }

    if (NULL != g_pf_usart_rx_callbacks[instance])
    {
        /* 普通接收完成没有实际长度信息，由上层按固定长度处理。 */
        g_pf_usart_rx_callbacks[instance](NULL, 0U);
    }

    return CORE_USART_OK;
}

/**
 * @brief 转发 HAL 发送完成事件到已注册的 TX 回调。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return USART 操作状态。
 * @note 仅供 App ISR Handler 调用。
 */
en_core_usart_status_t core_usart_irq_tx_complete(
    UART_HandleTypeDef *p_handle)
{
    en_core_usart_instance_t instance;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return CORE_USART_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    instance = core_usart_instance_from_handle(p_handle);
    if (CORE_USART_MAX == instance)
    {
        return CORE_USART_ERR_NOT_CONFIGURED;
    }

    if (NULL != g_pf_usart_tx_callbacks[instance])
    {
        g_pf_usart_tx_callbacks[instance]();
    }

    return CORE_USART_OK;
}

/**
 * @brief 转发 HAL 错误事件到已注册的错误回调。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return USART 操作状态。
 * @note 仅供 App ISR Handler 调用。
 */
en_core_usart_status_t core_usart_irq_error(
    UART_HandleTypeDef *p_handle)
{
    en_core_usart_instance_t instance;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return CORE_USART_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    instance = core_usart_instance_from_handle(p_handle);
    if (CORE_USART_MAX == instance)
    {
        return CORE_USART_ERR_NOT_CONFIGURED;
    }

    if (NULL != g_pf_usart_error_callbacks[instance])
    {
        g_pf_usart_error_callbacks[instance](p_handle->ErrorCode);
    }

    return CORE_USART_OK;
}

/**
 * @brief 转发 HAL 空闲线或 DMA 接收事件到 RX 回调。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @param[in] u16_size 本次实际接收字节数。
 * @return USART 操作状态。
 * @note 仅供 App ISR Handler 调用。
 */
en_core_usart_status_t core_usart_irq_rx_event(
    UART_HandleTypeDef *p_handle,
    uint16_t u16_size)
{
    en_core_usart_instance_t instance;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return CORE_USART_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    instance = core_usart_instance_from_handle(p_handle);
    if (CORE_USART_MAX == instance)
    {
        return CORE_USART_ERR_NOT_CONFIGURED;
    }

    if (NULL != g_pf_usart_rx_callbacks[instance])
    {
        g_pf_usart_rx_callbacks[instance](
            g_p_usart_rx_buffer[instance], u16_size);
    }

    return CORE_USART_OK;
}
//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
