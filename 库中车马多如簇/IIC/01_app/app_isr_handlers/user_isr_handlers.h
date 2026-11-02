/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.h
 *
 * @par dependencies
 * - i2c_bus.h
 * - i2c.h
 *
 * @author YA
 *
 * @brief 声明 I2C HAL 中断回调入口。
 *
 * Processing flow:
 *
 * HAL 中断回调 -> App ISR Handler -> I2C Bus 事件分发。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef IIC_USER_ISR_HANDLERS_H
#define IIC_USER_ISR_HANDLERS_H

//****************************** Includes begin ******************************//
#include "i2c_bus.h"

#include "i2c.h"
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 处理 I2C 内存读 DMA 完成中断。
 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *p_handle);

/**
 * @brief 处理 I2C 内存写 DMA 完成中断。
 */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *p_handle);

/**
 * @brief 处理 I2C 错误中断。
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *p_handle);

/**
 * @brief 处理 I2C 中止完成中断。
 */
void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *p_handle);
//************************* Function Prototypes end **************************//

#endif /* IIC_USER_ISR_HANDLERS_H */

//******************************* End of File ********************************//
