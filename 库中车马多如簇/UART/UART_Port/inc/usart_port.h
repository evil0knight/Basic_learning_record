#ifndef USART_PORT_H
#define USART_PORT_H

#include <stdint.h>

#include "main.h"
#include "usart.h"

/*
 * USART 平台抽象层（platform_mcu 层 Port）
 *
 * 统一 core_usart_* 接口，屏蔽 HAL 细节，支持阻塞收发、DMA 收发、
 * DMA + 空闲中断接收、回调注册。静态表在 usart_port_config.h 配置。
 */

/* USART 状态码 */
typedef enum
{
    CORE_USART_OK = 0,
    CORE_USART_ERROR,
    CORE_USART_BUSY,
    CORE_USART_TIMEOUT
} en_core_usart_status_t;

/* USART 实例枚举 */
typedef enum
{
    CORE_USART1 = 0,
    CORE_USART_MAX
} en_core_usart_instance_t;

/* 单实例配置（在 usart_port_config.h 静态表里填） */
typedef struct
{
    UART_HandleTypeDef *handle;
    uint32_t baudrate;
    uint32_t timeout;
} st_usart_config_t;

/* RX 完成回调：data=接收缓冲，size=本次收到字节数；DMA 完成时 data=NULL, size=0 */
typedef void (*usart_rx_callback_t)(uint8_t *data, uint16_t size);
/* TX 完成回调 */
typedef void (*usart_tx_callback_t)(void);
/* 错误回调：error_code = HAL 错误码 */
typedef void (*usart_error_callback_t)(uint32_t error_code);

#define CORE_USART_DEFAULT_TIMEOUT      1000U
#define CORE_USART_MAX_BUFFER_SIZE      1024U

/* 初始化：调 MX_USARTx_UART_Init + 清回调表 */
en_core_usart_status_t core_usart_init(void);

/* 阻塞发送 */
en_core_usart_status_t core_usart_transmit(en_core_usart_instance_t instance,
                                           const uint8_t *data,
                                           uint16_t size,
                                           uint32_t timeout);

/* 阻塞接收 */
en_core_usart_status_t core_usart_receive(en_core_usart_instance_t instance,
                                          uint8_t *data,
                                          uint16_t size,
                                          uint32_t timeout);

/* DMA 发送 */
en_core_usart_status_t core_usart_transmit_dma(en_core_usart_instance_t instance,
                                               const uint8_t *data,
                                               uint16_t size);

/* DMA 接收 */
en_core_usart_status_t core_usart_receive_dma(en_core_usart_instance_t instance,
                                              uint8_t *data,
                                              uint16_t size);

/* DMA + 空闲中断接收：一帧收完触发 rx_callback */
en_core_usart_status_t core_usart_receive_to_idle_dma(en_core_usart_instance_t instance,
                                                      uint8_t *data,
                                                      uint16_t size);

/* 注册 RX 回调 */
en_core_usart_status_t core_usart_register_rx_callback(en_core_usart_instance_t instance,
                                                       usart_rx_callback_t callback);

/* 注册 TX 回调 */
en_core_usart_status_t core_usart_register_tx_callback(en_core_usart_instance_t instance,
                                                       usart_tx_callback_t callback);

/* 注册错误回调 */
en_core_usart_status_t core_usart_register_error_callback(en_core_usart_instance_t instance,
                                                          usart_error_callback_t callback);

/* 取 HAL 句柄（特殊场景直接操作 HAL 用） */
UART_HandleTypeDef *core_usart_get_handle(en_core_usart_instance_t instance);

#endif /* USART_PORT_H */
