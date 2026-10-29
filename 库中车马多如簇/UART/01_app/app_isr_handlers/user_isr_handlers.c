/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.c
 *
 * @par dependencies
 * - user_isr_handlers.h
 * - usart_port.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief UART HAL 中断回调入口，将 HAL 事件转发到 USART Port。
 *
 * Processing flow:
 *
 * HAL 中断 -> 本文件回调入口 -> core_usart_irq_* -> 用户回调。
 *
 * @version V1.0 2026-10-27
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "user_isr_handlers.h"

#include "usart_port.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//**************************** ISR Handlers begin ****************************//
/**
 * @brief 处理 UART 接收完成中断
 *        普通cpu接收,搬运依靠cpu搬运到缓冲区,缓冲区满了就中断
 * @param[in] p_handle HAL UART 句柄。
 * @return void HAL 回调固定签名。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_usart_irq_rx_complete(p_handle);
}

/**
 * @brief 处理 UART 发送完成中断。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return void HAL 回调固定签名。
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_usart_irq_tx_complete(p_handle);
}

/**
 * @brief 处理 UART 错误中断。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @return void HAL 回调固定签名。
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_usart_irq_error(p_handle);
}

/**
 * @brief 处理 UART 空闲线或 DMA 缓冲区接收满事件。
 *
 * @param[in] p_handle HAL UART 句柄。
 * @param[in] u16_size 本次实际接收字节数。
 * @return void HAL 回调固定签名。
 */
void HAL_UARTEx_RxEventCallback(
    UART_HandleTypeDef *p_handle,
    uint16_t u16_size)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_usart_irq_rx_event(p_handle, u16_size);
}
//***************************** ISR Handlers end *****************************//

//******************************* End of File ********************************//
