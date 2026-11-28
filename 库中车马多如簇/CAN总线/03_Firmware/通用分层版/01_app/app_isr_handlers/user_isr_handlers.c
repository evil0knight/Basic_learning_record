/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.c
 *
 * @par dependencies
 * - user_isr_handlers.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 转发 FDCAN 或 bxCAN 的 HAL 中断事件。
 *
 * Processing flow:
 *
 * 参数检查 -> 调用 HAL CAN 或分发回调 -> 状态码转换 -> 返回。
 *
 * @version V1.0 2026-11-23
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "user_isr_handlers.h"
#include <stddef.h>
//******************************* Includes end *******************************//

//**************************** ISR Handlers begin ****************************//
#if CORE_CAN_USE_FDCAN
/**
 * @brief 转发 HAL FDCAN RxFifo0 事件。
 * @return void HAL 固定回调签名。
 */
void HAL_FDCAN_RxFifo0Callback(
    FDCAN_HandleTypeDef *p_handle,
    uint32_t rx_interrupts)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    if (0U != (rx_interrupts & FDCAN_IT_RX_FIFO0_NEW_MESSAGE))
    {
        (void)core_can_irq_rx_pending(p_handle, CORE_CAN_FIFO0);
    }
}

/**
 * @brief 转发 HAL FDCAN RxFifo1 事件。
 * @return void HAL 固定回调签名。
 */
void HAL_FDCAN_RxFifo1Callback(
    FDCAN_HandleTypeDef *p_handle,
    uint32_t rx_interrupts)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    if (0U != (rx_interrupts & FDCAN_IT_RX_FIFO1_NEW_MESSAGE))
    {
        (void)core_can_irq_rx_pending(p_handle, CORE_CAN_FIFO1);
    }
}

/**
 * @brief 转发 HAL FDCAN TxBufferComplete 事件。
 * @return void HAL 固定回调签名。
 */
void HAL_FDCAN_TxBufferCompleteCallback(
    FDCAN_HandleTypeDef *p_handle,
    uint32_t buffer_mask)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    if (0U != (buffer_mask))
    {
        (void)core_can_irq_tx_complete(p_handle, buffer_mask);
    }
}

/**
 * @brief 转发 HAL FDCAN Error 事件。
 * @return void HAL 固定回调签名。
 */
void HAL_FDCAN_ErrorCallback(
    FDCAN_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_can_irq_error(p_handle);
}

/**
 * @brief 转发 HAL FDCAN ErrorStatus 事件。
 * @return void HAL 固定回调签名。
 */
void HAL_FDCAN_ErrorStatusCallback(
    FDCAN_HandleTypeDef *p_handle,
    uint32_t status_flags)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    if (0U != (status_flags))
    {
        (void)core_can_irq_error_status(p_handle, status_flags);
    }
}
#else
/**
 * @brief 转发 HAL CAN TxMailbox0Complete 事件。
 * @return void HAL 固定回调签名。
 */
void HAL_CAN_TxMailbox0CompleteCallback(
    CAN_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_can_irq_tx_complete(p_handle, CAN_TX_MAILBOX0);
}

/**
 * @brief 转发 HAL CAN TxMailbox1Complete 事件。
 * @return void HAL 固定回调签名。
 */
void HAL_CAN_TxMailbox1CompleteCallback(
    CAN_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_can_irq_tx_complete(p_handle, CAN_TX_MAILBOX1);
}

/**
 * @brief 转发 HAL CAN TxMailbox2Complete 事件。
 * @return void HAL 固定回调签名。
 */
void HAL_CAN_TxMailbox2CompleteCallback(
    CAN_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_can_irq_tx_complete(p_handle, CAN_TX_MAILBOX2);
}

/**
 * @brief 转发 HAL CAN RxFifo0MsgPending 事件。
 * @return void HAL 固定回调签名。
 */
void HAL_CAN_RxFifo0MsgPendingCallback(
    CAN_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_can_irq_rx_pending(p_handle, CORE_CAN_FIFO0);
}

/**
 * @brief 转发 HAL CAN RxFifo1MsgPending 事件。
 * @return void HAL 固定回调签名。
 */
void HAL_CAN_RxFifo1MsgPendingCallback(
    CAN_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_can_irq_rx_pending(p_handle, CORE_CAN_FIFO1);
}

/**
 * @brief 转发 HAL CAN Error 事件。
 * @return void HAL 固定回调签名。
 */
void HAL_CAN_ErrorCallback(
    CAN_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)core_can_irq_error(p_handle);
}
#endif
//***************************** ISR Handlers end *****************************//

//******************************* End of File ********************************//
