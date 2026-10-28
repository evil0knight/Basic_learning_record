/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file usart_port.h
 *
 * @par dependencies
 * - stdint.h
 * - main.h
 * - usart.h
 *
 * @author YA
 *
 * @brief USART 平台抽象层接口，提供阻塞/DMA 收发及回调注册。
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

#ifndef USART_PORT_H
#define USART_PORT_H

//****************************** Includes begin ******************************//
#include <stdint.h>

#include "main.h"
#include "usart.h"
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
#define CORE_USART_DEFAULT_TIMEOUT      (1000U)
#define CORE_USART_MAX_BUFFER_SIZE      (1024U)
//******************************* Defines end ********************************//

//******************************* Enums begin ********************************//
/**
 * @brief USART 操作状态。
 */
typedef enum
{
    CORE_USART_OK = 0,
    CORE_USART_ERROR,
    CORE_USART_BUSY,
    CORE_USART_TIMEOUT,
    CORE_USART_ERR_ARGUMENT,
    CORE_USART_ERR_NOT_CONFIGURED
} en_core_usart_status_t;

/**
 * @brief USART 逻辑实例编号。
 */
typedef enum
{
    CORE_USART1 = 0,
    CORE_USART_MAX
} en_core_usart_instance_t;
//******************************** Enums end *********************************//

//****************************** Typedefs begin ******************************//
/**
 * @brief RX 完成回调：data 为接收缓冲，size 为本次接收字节数。
 */
typedef void (*usart_rx_callback_t)(uint8_t *p_data, uint16_t size);

/**
 * @brief TX 完成回调。
 */
typedef void (*usart_tx_callback_t)(void);

/**
 * @brief 错误回调：error_code 为 HAL 错误码。
 */
typedef void (*usart_error_callback_t)(uint32_t error_code);
//******************************* Typedefs end *******************************//

//****************************** Structs begin *******************************//
/**
 * @brief 单个 USART 实例的静态配置。
 */
typedef struct
{
    UART_HandleTypeDef *p_handle;
    uint32_t baudrate;
    uint32_t timeout;
} st_usart_config_t;
//******************************* Structs end ********************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 初始化 USART 平台层并清空回调状态。
 *
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_init(void);

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
    uint32_t timeout);

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
    uint32_t timeout);

/**
 * @brief 以 DMA 方式发送数据。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[in] p_data 发送缓冲区。
 * @param[in] size 发送字节数。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_transmit_dma(
    en_core_usart_instance_t instance,
    const uint8_t *p_data,
    uint16_t size);

/**
 * @brief 以 DMA 方式启动一轮接收。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[out] p_data 接收缓冲区。
 * @param[in] size 接收缓冲区大小。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_receive_dma(
    en_core_usart_instance_t instance,
    uint8_t *p_data,
    uint16_t size);

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
    uint16_t size);

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
    uint32_t timeout);

/**
 * @brief 注册 RX 完成回调。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[in] pf_callback 回调函数指针。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_register_rx_callback(
    en_core_usart_instance_t instance,
    usart_rx_callback_t pf_callback);

/**
 * @brief 注册 TX 完成回调。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[in] pf_callback 回调函数指针。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_register_tx_callback(
    en_core_usart_instance_t instance,
    usart_tx_callback_t pf_callback);

/**
 * @brief 注册错误回调。
 *
 * @param[in] instance 逻辑串口实例。
 * @param[in] pf_callback 回调函数指针。
 * @return USART 操作状态。
 */
en_core_usart_status_t core_usart_register_error_callback(
    en_core_usart_instance_t instance,
    usart_error_callback_t pf_callback);

/**
 * @brief 获取底层 HAL UART 句柄。
 *
 * @param[in] instance 逻辑串口实例。
 * @return HAL 句柄；实例非法时返回 NULL。
 */
UART_HandleTypeDef *core_usart_get_handle(
    en_core_usart_instance_t instance);

/**
 * @brief 转发 HAL 接收完成事件到已注册的 RX 回调。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return USART 操作状态。
 * @note 仅供 App ISR Handler 调用。
 */
en_core_usart_status_t core_usart_irq_rx_complete(
    UART_HandleTypeDef *p_handle);

/**
 * @brief 转发 HAL 发送完成事件到已注册的 TX 回调。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return USART 操作状态。
 * @note 仅供 App ISR Handler 调用。
 */
en_core_usart_status_t core_usart_irq_tx_complete(
    UART_HandleTypeDef *p_handle);

/**
 * @brief 转发 HAL 错误事件到已注册的错误回调。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return USART 操作状态。
 * @note 仅供 App ISR Handler 调用。
 */
en_core_usart_status_t core_usart_irq_error(
    UART_HandleTypeDef *p_handle);

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
    uint16_t u16_size);
//************************* Function Prototypes end **************************//

#endif /* USART_PORT_H */
