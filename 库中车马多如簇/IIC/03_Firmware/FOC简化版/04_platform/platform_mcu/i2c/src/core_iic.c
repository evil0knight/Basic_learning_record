/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file core_iic.c
 *
 * @par dependencies
 * - core_iic.h
 * - i2c.h
 *
 * @author YA
 *
 * @brief 单路硬件 I2C 的直接 HAL 封装实现。
 *
 * Processing flow:
 *
 * Wrapper 调用 core_iic_* -> 直接调用 hi2c1 的 HAL 接口
 * -> 将 HAL 状态映射为 CORE_IIC_* 后返回。
 *
 * @version V1.0
 *
 * @note 仅保留一层封装，不提供逻辑端口、请求队列或回调注册。
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "core_iic.h"

#include "i2c.h"
//******************************* Includes end *******************************//

//************************** Static Functions begin **************************//
/**
 * @brief 将 HAL 传输结果映射为单路 I2C 状态。
 *
 * @param[in] en_status : HAL 返回状态。
 *
 * @return en_core_iic_status_t : 对应的统一状态。
 */
static en_core_iic_status_t core_iic_hal_status(HAL_StatusTypeDef en_status)
{
    switch (en_status)
    {
        case HAL_OK:
            return CORE_IIC_OK;

        case HAL_BUSY:
            return CORE_IIC_ERR_BUSY;

        case HAL_TIMEOUT:
            return CORE_IIC_ERR_TIMEOUT;

        default:
            return (0U != (hi2c1.ErrorCode & HAL_I2C_ERROR_AF))
                       ? CORE_IIC_ERR_NACK
                       : CORE_IIC_ERROR;
    }
}
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 检查硬件 I2C 句柄是否已初始化。
 *
 * Steps:
 *  1. 读取 hi2c1 的 HAL 状态。
 *  2. 仅当状态为 READY 时返回成功。
 *
 * @return en_core_iic_status_t : 就绪返回 CORE_IIC_OK，否则返回错误。
 */
en_core_iic_status_t core_iic_init(void)
{
    if (HAL_I2C_STATE_READY != HAL_I2C_GetState(&hi2c1))
    {
        return CORE_IIC_ERROR;
    }

    return CORE_IIC_OK;
}

/**
 * @brief 从指定寄存器同步读取连续数据,堵塞
 *
 * Steps:
 *  1. 检查读取长度非零。
 *  2. 直接调用 HAL 内存读接口并映射状态。
 *
 * @param[in]  u8_device_address_7bit : 未左移的七位设备地址。
 * @param[in]  u8_register            : 设备内部八位寄存器地址。
 * @param[out] p_data                 : 接收缓冲区，由调用者保证有效。
 * @param[in]  u16_size               : 读取字节数。
 *
 * @return en_core_iic_status_t : 参数、接口或传输结果。
 */
en_core_iic_status_t core_iic_read(
    uint8_t   u8_device_address_7bit,
    uint8_t   u8_register,
    uint8_t  *p_data,
    uint16_t  u16_size)
{
    //-------------------------参数检查 begin-------------------------//
    if (0U == u16_size)
    {
        return CORE_IIC_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return core_iic_hal_status(HAL_I2C_Mem_Read(
        &hi2c1,
        (uint16_t)((uint16_t)u8_device_address_7bit << 1U),
        u8_register,
        I2C_MEMADD_SIZE_8BIT,
        p_data,
        u16_size,
        CORE_IIC_TIMEOUT_MS));
}

/**
 * @brief 向指定寄存器同步写入连续数据。
 *
 * Steps:
 *  1. 检查写入长度非零。
 *  2. 直接调用 HAL 内存写接口并映射状态。
 *
 * @param[in] u8_device_address_7bit : 未左移的七位设备地址。
 * @param[in] u8_register            : 设备内部八位寄存器地址。
 * @param[in] p_data                 : 发送缓冲区，由调用者保证有效。
 * @param[in] u16_size               : 写入字节数。
 *
 * @return en_core_iic_status_t : 参数、接口或传输结果。
 */
en_core_iic_status_t core_iic_write(
    uint8_t        u8_device_address_7bit,
    uint8_t        u8_register,
    const uint8_t *p_data,
    uint16_t       u16_size)
{
    //-------------------------参数检查 begin-------------------------//
    if (0U == u16_size)
    {
        return CORE_IIC_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return core_iic_hal_status(HAL_I2C_Mem_Write(
        &hi2c1,
        (uint16_t)((uint16_t)u8_device_address_7bit << 1U),
        u8_register,
        I2C_MEMADD_SIZE_8BIT,
        (uint8_t *)p_data,
        u16_size,
        CORE_IIC_TIMEOUT_MS));
}

/**
 * @brief 在一次同步事务中读取高、低字节，失败时不更新输出。
 *
 * Steps:
 *  1. 通过 core_iic_read 读取两个字节。
 *  2. 仅在读取成功时组合并写回 16 位值。
 *
 * @param[in]  u8_device_address_7bit : 未左移的七位设备地址。
 * @param[in]  u8_register            : 高字节寄存器地址。
 * @param[out] p_u16_value            : 16 位寄存器值，由调用者保证有效。
 *
 * @return en_core_iic_status_t : 参数、接口或传输结果。
 */
en_core_iic_status_t core_iic_read_word(
    uint8_t   u8_device_address_7bit,
    uint8_t   u8_register,
    uint16_t *p_u16_value)
{
    uint8_t              a_u8_data[2] = {0U, 0U};
    en_core_iic_status_t en_status;

    en_status = core_iic_read(
        u8_device_address_7bit, u8_register, a_u8_data, 2U);
    if (CORE_IIC_OK == en_status)
    {
        *p_u16_value = (uint16_t)(((uint16_t)a_u8_data[0] << 8U) |
                                  (uint16_t)a_u8_data[1]);
    }

    return en_status;
}

/**
 * @brief 按掩码修改双字节寄存器，保留其余位并等待至少 1ms。
 *
 * Steps:
 *  1. 读回当前双字节值，失败则直接返回。
 *  2. 按掩码合成新值并写回。
 *  3. 写入成功后等待寄存器更新。
 *
 * @param[in] u8_device_address_7bit : 未左移的七位设备地址。
 * @param[in] u8_register            : 高字节寄存器地址。
 * @param[in] u16_mask               : 可修改位。
 * @param[in] u16_value              : 要写入的位值。
 *
 * @return en_core_iic_status_t : 参数、传输或延时结果。
 */
en_core_iic_status_t core_iic_update_word(
    uint8_t  u8_device_address_7bit,
    uint8_t  u8_register,
    uint16_t u16_mask,
    uint16_t u16_value)
{
    uint16_t             u16_current = 0U;
    uint8_t              a_u8_data[2];
    en_core_iic_status_t en_status;

    en_status = core_iic_read_word(
        u8_device_address_7bit, u8_register, &u16_current);
    if (CORE_IIC_OK != en_status)
    {
        return en_status;
    }

    u16_current = (uint16_t)((u16_current & (uint16_t)(~u16_mask)) |
                             (u16_value & u16_mask));
    a_u8_data[0] = (uint8_t)(u16_current >> 8U);
    a_u8_data[1] = (uint8_t)u16_current;
    en_status = core_iic_write(
        u8_device_address_7bit, u8_register, a_u8_data, 2U);
    if (CORE_IIC_OK != en_status)
    {
        return en_status;
    }

    HAL_Delay(CORE_IIC_UPDATE_DELAY_MS);
    return CORE_IIC_OK;
}

/**
 * @brief 启动一次指定寄存器的 DMA 读。
 *
 * Steps:
 *  1. 检查读取长度非零。
 *  2. 直接调用 HAL 内存读 DMA 接口并映射启动状态。
 *
 * @param[in]  u8_device_address_7bit : 未左移的七位设备地址。
 * @param[in]  u8_register            : 设备内部八位寄存器地址。
 * @param[out] p_data                 : 接收缓冲区，由调用者保证有效。
 * @param[in]  u16_size               : 读取字节数。
 *
 * @return en_core_iic_status_t : 启动结果；CORE_IIC_OK 只表示已启动。
 */
en_core_iic_status_t core_iic_read_dma(
    uint8_t   u8_device_address_7bit,
    uint8_t   u8_register,
    uint8_t  *p_data,
    uint16_t  u16_size)
{
    //-------------------------参数检查 begin-------------------------//
    if (0U == u16_size)
    {
        return CORE_IIC_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return core_iic_hal_status(HAL_I2C_Mem_Read_DMA(
        &hi2c1,
        (uint16_t)((uint16_t)u8_device_address_7bit << 1U),
        u8_register,
        I2C_MEMADD_SIZE_8BIT,
        p_data,
        u16_size));
}

/**
 * @brief 中止当前 DMA 传输。
 *
 * @param[in] u8_device_address_7bit : 未左移的七位设备地址。
 *
 * @return en_core_iic_status_t : 中止请求结果。
 */
en_core_iic_status_t core_iic_abort(uint8_t u8_device_address_7bit)
{
    return core_iic_hal_status(HAL_I2C_Master_Abort_IT(
        &hi2c1,
        (uint16_t)((uint16_t)u8_device_address_7bit << 1U)));
}
//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
