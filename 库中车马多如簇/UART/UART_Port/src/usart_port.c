#include "usart_port.h"

#include "usart_port_config.h"

/*
 * USART 平台抽象层实现
 *
 * core_usart_* 统一接口，内部转 HAL。DMA 收发、空闲中断、回调都收敛在这里，
 * 上层（OTA、Ymodem、业务）只调 core_usart_*，不直接碰 HAL。
 */

static usart_rx_callback_t g_usart_rx_callbacks[CORE_USART_MAX] = {NULL};
static usart_tx_callback_t g_usart_tx_callbacks[CORE_USART_MAX] = {NULL};
static usart_error_callback_t g_usart_error_callbacks[CORE_USART_MAX] = {NULL};

/* 最近一次 receive_to_idle_dma 的缓冲，空闲中断回调时传回给上层 */
static uint8_t *g_usart_rx_buffer[CORE_USART_MAX] = {NULL};

static en_core_usart_status_t core_usart_from_hal(HAL_StatusTypeDef hal_status)
{
    switch (hal_status)
    {
        case HAL_OK:
            return CORE_USART_OK;
        case HAL_BUSY:
            return CORE_USART_BUSY;
        case HAL_TIMEOUT:
            return CORE_USART_TIMEOUT;
        case HAL_ERROR:
        default:
            return CORE_USART_ERROR;
    }
}

static en_core_usart_instance_t core_usart_instance_from_handle(UART_HandleTypeDef *huart)
{
    uint32_t i;

    if (huart == NULL)
    {
        return CORE_USART_MAX;
    }

    for (i = 0U; i < CORE_USART_MAX; i++)
    {
        if (g_usart_configs[i].handle == huart)
        {
            return (en_core_usart_instance_t)i;
        }
    }

    return CORE_USART_MAX;
}

static en_core_usart_status_t core_usart_validate(en_core_usart_instance_t instance,
                                                  const void *data,
                                                  uint16_t size)
{
    if (instance >= CORE_USART_MAX)
    {
        return CORE_USART_ERROR;
    }
    if ((data == NULL) || (size == 0U))
    {
        return CORE_USART_ERROR;
    }
    if (g_usart_configs[instance].handle == NULL)
    {
        return CORE_USART_ERROR;
    }

    return CORE_USART_OK;
}

/* 初始化：调 CubeMX 生成的 MX_USARTx_UART_Init + 清回调表 */
en_core_usart_status_t core_usart_init(void)
{
    uint32_t i;

    MX_USART1_UART_Init();

    for (i = 0U; i < CORE_USART_MAX; i++)
    {
        g_usart_rx_callbacks[i] = NULL;
        g_usart_tx_callbacks[i] = NULL;
        g_usart_error_callbacks[i] = NULL;
        g_usart_rx_buffer[i] = NULL;
    }

    return CORE_USART_OK;
}

/* 阻塞发送 */
en_core_usart_status_t core_usart_transmit(en_core_usart_instance_t instance,
                                           const uint8_t *data,
                                           uint16_t size,
                                           uint32_t timeout)
{
    en_core_usart_status_t status = core_usart_validate(instance, data, size);

    if (status != CORE_USART_OK)
    {
        return status;
    }

    return core_usart_from_hal(
        HAL_UART_Transmit(g_usart_configs[instance].handle,
                          (uint8_t *)data, size, timeout));
}

/* 阻塞接收 */
en_core_usart_status_t core_usart_receive(en_core_usart_instance_t instance,
                                          uint8_t *data,
                                          uint16_t size,
                                          uint32_t timeout)
{
    en_core_usart_status_t status = core_usart_validate(instance, data, size);

    if (status != CORE_USART_OK)
    {
        return status;
    }

    return core_usart_from_hal(
        HAL_UART_Receive(g_usart_configs[instance].handle, data, size, timeout));
}

/* DMA 发送 */
en_core_usart_status_t core_usart_transmit_dma(en_core_usart_instance_t instance,
                                               const uint8_t *data,
                                               uint16_t size)
{
    en_core_usart_status_t status = core_usart_validate(instance, data, size);

    if (status != CORE_USART_OK)
    {
        return status;
    }

    return core_usart_from_hal(
        HAL_UART_Transmit_DMA(g_usart_configs[instance].handle,
                              (uint8_t *)data, size));
}

/* DMA 接收（循环模式，完成走 HAL_UART_RxCpltCallback） */
en_core_usart_status_t core_usart_receive_dma(en_core_usart_instance_t instance,
                                              uint8_t *data,
                                              uint16_t size)
{
    en_core_usart_status_t status = core_usart_validate(instance, data, size);

    if (status != CORE_USART_OK)
    {
        return status;
    }

    return core_usart_from_hal(
        HAL_UART_Receive_DMA(g_usart_configs[instance].handle, data, size));
}

/* DMA + 空闲中断接收：一帧收完触发 rx_callback(data, size) */
en_core_usart_status_t core_usart_receive_to_idle_dma(en_core_usart_instance_t instance,
                                                      uint8_t *data,
                                                      uint16_t size)
{
    UART_HandleTypeDef *handle;
    HAL_StatusTypeDef hal_status;
    en_core_usart_status_t status = core_usart_validate(instance, data, size);

    if (status != CORE_USART_OK)
    {
        return status;
    }

    handle = g_usart_configs[instance].handle;
    g_usart_rx_buffer[instance] = data;

    hal_status = HAL_UARTEx_ReceiveToIdle_DMA(handle, data, size);
    if ((hal_status == HAL_OK) && (handle->hdmarx != NULL))
    {
        /* 关半传输中断，只保留全传输和空闲中断 */
        __HAL_DMA_DISABLE_IT(handle->hdmarx, DMA_IT_HT);
    }

    return core_usart_from_hal(hal_status);
}

/* 注册 RX 回调 */
en_core_usart_status_t core_usart_register_rx_callback(en_core_usart_instance_t instance,
                                                       usart_rx_callback_t callback)
{
    if (instance >= CORE_USART_MAX)
    {
        return CORE_USART_ERROR;
    }

    g_usart_rx_callbacks[instance] = callback;
    return CORE_USART_OK;
}

/* 注册 TX 回调 */
en_core_usart_status_t core_usart_register_tx_callback(en_core_usart_instance_t instance,
                                                       usart_tx_callback_t callback)
{
    if (instance >= CORE_USART_MAX)
    {
        return CORE_USART_ERROR;
    }

    g_usart_tx_callbacks[instance] = callback;
    return CORE_USART_OK;
}

/* 注册错误回调 */
en_core_usart_status_t core_usart_register_error_callback(en_core_usart_instance_t instance,
                                                          usart_error_callback_t callback)
{
    if (instance >= CORE_USART_MAX)
    {
        return CORE_USART_ERROR;
    }

    g_usart_error_callbacks[instance] = callback;
    return CORE_USART_OK;
}

/* 取 HAL 句柄 */
UART_HandleTypeDef *core_usart_get_handle(en_core_usart_instance_t instance)
{
    return (instance < CORE_USART_MAX) ? g_usart_configs[instance].handle : NULL;
}

/* ===== HAL 回调桥 ===== */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    en_core_usart_instance_t instance = core_usart_instance_from_handle(huart);

    if ((instance < CORE_USART_MAX) && (g_usart_rx_callbacks[instance] != NULL))
    {
        g_usart_rx_callbacks[instance](NULL, 0U);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    en_core_usart_instance_t instance = core_usart_instance_from_handle(huart);

    if ((instance < CORE_USART_MAX) && (g_usart_tx_callbacks[instance] != NULL))
    {
        g_usart_tx_callbacks[instance]();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    en_core_usart_instance_t instance = core_usart_instance_from_handle(huart);

    if ((instance < CORE_USART_MAX) &&
        (g_usart_error_callbacks[instance] != NULL) && (huart != NULL))
    {
        g_usart_error_callbacks[instance](huart->ErrorCode);
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    en_core_usart_instance_t instance = core_usart_instance_from_handle(huart);

    if ((instance < CORE_USART_MAX) && (g_usart_rx_callbacks[instance] != NULL))
    {
        g_usart_rx_callbacks[instance](g_usart_rx_buffer[instance], Size);
    }
}
