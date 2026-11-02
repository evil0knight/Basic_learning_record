/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file user_isr_handlers.c
 *
 * @par dependencies
 * - user_isr_handlers.h
 * - i2c_bus.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief I2C HAL 中断回调入口，将 HAL 事件转发到 I2C Bus。
 *
 * Processing flow:
 *
 * HAL 中断 -> 本文件回调入口 -> i2c_bus_irq_* -> 注册的完成回调。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "user_isr_handlers.h"

#include "i2c_bus.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//**************************** ISR Handlers begin ****************************//
/**
 * @brief 处理 I2C 内存读 DMA 完成中断。
 *
 * Steps:
 *  1. 检查 HAL 句柄，空指针时直接返回。
 *  2. 调用 i2c_bus_irq_mem_rx_complete，由 Bus 分发事件。
 *
 * @param[in]  p_handle : HAL I2C 句柄。
 *
 * @return void : HAL 回调固定签名。
 *
 * @note 在中断上下文执行，不在入口中处理数据。
 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)i2c_bus_irq_mem_rx_complete(p_handle);
}

/**
 * @brief 处理 I2C 内存写 DMA 完成中断。
 *
 * Steps:
 *  1. 检查 HAL 句柄，空指针时直接返回。
 *  2. 调用 i2c_bus_irq_mem_tx_complete，由 Bus 分发事件。
 *
 * @param[in]  p_handle : HAL I2C 句柄。
 *
 * @return void : HAL 回调固定签名。
 *
 * @note 在中断上下文执行。
 */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)i2c_bus_irq_mem_tx_complete(p_handle);
}

/**
 * @brief 处理 I2C 错误中断。
 *
 * Steps:
 *  1. 检查 HAL 句柄，空指针时直接返回。
 *  2. 调用 i2c_bus_irq_error，由 Bus 分发错误。
 *
 * @param[in]  p_handle : HAL I2C 句柄。
 *
 * @return void : HAL 回调固定签名。
 *
 * @note 在中断上下文执行。
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)i2c_bus_irq_error(p_handle);
}

/**
 * @brief 处理 I2C 中止完成中断。
 *
 * Steps:
 *  1. 检查 HAL 句柄，空指针时直接返回。
 *  2. 调用 i2c_bus_irq_abort_complete，由 Bus 清理总线状态。
 *
 * @param[in]  p_handle : HAL I2C 句柄。
 *
 * @return void : HAL 回调固定签名。
 *
 * @note 在中断上下文执行。
 */
void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)i2c_bus_irq_abort_complete(p_handle);
}
//***************************** ISR Handlers end *****************************//

//******************************* End of File ********************************//
