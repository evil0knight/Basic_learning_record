/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.h
 *
 * @par dependencies
 * - can_port.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 声明所选 CAN 后端的 HAL 回调转发入口。
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

#ifndef USER_ISR_HANDLERS_H
#define USER_ISR_HANDLERS_H

//****************************** Includes begin ******************************//
#include "can_port.h"
#include <stdint.h>
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//
#if CORE_CAN_USE_FDCAN
/** @brief HAL FDCAN RxFifo0 回调入口。 */
void HAL_FDCAN_RxFifo0Callback(
    FDCAN_HandleTypeDef *p_handle,
    uint32_t rx_interrupts);

/** @brief HAL FDCAN RxFifo1 回调入口。 */
void HAL_FDCAN_RxFifo1Callback(
    FDCAN_HandleTypeDef *p_handle,
    uint32_t rx_interrupts);

/** @brief HAL FDCAN TxBufferComplete 回调入口。 */
void HAL_FDCAN_TxBufferCompleteCallback(
    FDCAN_HandleTypeDef *p_handle,
    uint32_t buffer_mask);

/** @brief HAL FDCAN Error 回调入口。 */
void HAL_FDCAN_ErrorCallback(
    FDCAN_HandleTypeDef *p_handle);

/** @brief HAL FDCAN ErrorStatus 回调入口。 */
void HAL_FDCAN_ErrorStatusCallback(
    FDCAN_HandleTypeDef *p_handle,
    uint32_t status_flags);
#else
/** @brief HAL CAN TxMailbox0Complete 回调入口。 */
void HAL_CAN_TxMailbox0CompleteCallback(
    CAN_HandleTypeDef *p_handle);

/** @brief HAL CAN TxMailbox1Complete 回调入口。 */
void HAL_CAN_TxMailbox1CompleteCallback(
    CAN_HandleTypeDef *p_handle);

/** @brief HAL CAN TxMailbox2Complete 回调入口。 */
void HAL_CAN_TxMailbox2CompleteCallback(
    CAN_HandleTypeDef *p_handle);

/** @brief HAL CAN RxFifo0MsgPending 回调入口。 */
void HAL_CAN_RxFifo0MsgPendingCallback(
    CAN_HandleTypeDef *p_handle);

/** @brief HAL CAN RxFifo1MsgPending 回调入口。 */
void HAL_CAN_RxFifo1MsgPendingCallback(
    CAN_HandleTypeDef *p_handle);

/** @brief HAL CAN Error 回调入口。 */
void HAL_CAN_ErrorCallback(
    CAN_HandleTypeDef *p_handle);
#endif
//************************* Function Prototypes end **************************//

#endif /* USER_ISR_HANDLERS_H */

//******************************* End of File ********************************//
