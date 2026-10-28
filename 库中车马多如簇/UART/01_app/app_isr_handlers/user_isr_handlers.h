/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.h
 *
 * @par dependencies
 * - stdint.h
 * - usart.h
 *
 * @author YA
 *
 * @brief 声明 UART HAL 中断回调入口。
 *
 * Processing flow:
 *
 * HAL 中断回调 -> App ISR Handler -> USART Port 事件分发。
 *
 * @version V1.0 2026-10-27
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef UART_USER_ISR_HANDLERS_H
#define UART_USER_ISR_HANDLERS_H

//****************************** Includes begin ******************************//
#include <stdint.h>

#include "usart.h"
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 处理 UART 接收完成中断。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return void HAL 回调固定签名。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *p_handle);

/**
 * @brief 处理 UART 发送完成中断。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return void HAL 回调固定签名。
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *p_handle);

/**
 * @brief 处理 UART 错误中断。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return void HAL 回调固定签名。
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *p_handle);

/**
 * @brief 处理 UART 空闲线或 DMA 缓冲区接收满事件。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @param[in] u16_size 本次实际接收字节数。
 * @return void HAL 回调固定签名。
 */
void HAL_UARTEx_RxEventCallback(
    UART_HandleTypeDef *p_handle,
    uint16_t u16_size);
//************************* Function Prototypes end **************************//

#endif /* UART_USER_ISR_HANDLERS_H */
