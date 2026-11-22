/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file core_iic.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief 声明单路硬件 I2C 的直接 HAL 封装接口。
 *
 * Processing flow:
 *
 * Wrapper 调用 core_iic_* -> 直接访问 hi2c1 的 HAL 同步/DMA 接口
 * -> 返回统一状态。
 *
 * @version V1.0
 *
 * @note 仅服务编码器所在的单路硬件 I2C，不提供逻辑端口与队列。
 *       所有指针由调用者保证有效。
 *
 *****************************************************************************/

#ifndef CORE_IIC_H
#define CORE_IIC_H

//****************************** Includes begin ******************************//
#include <stdint.h>
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
/* 同步寄存器读写的 HAL 超时，单位 ms。 */
#define CORE_IIC_TIMEOUT_MS        (10U)
/* 双字节寄存器写入后的最小等待，单位 ms。 */
#define CORE_IIC_UPDATE_DELAY_MS   (1U)
//******************************* Defines end ********************************//

//******************************* Enums begin ********************************//
/**
 * @brief 单路硬件 I2C 操作状态。
 */
typedef enum
{
    CORE_IIC_OK = 0,
    CORE_IIC_ERROR,
    CORE_IIC_ERR_ARGUMENT,
    CORE_IIC_ERR_BUSY,
    CORE_IIC_ERR_TIMEOUT,
    CORE_IIC_ERR_NACK
} en_core_iic_status_t;
//******************************** Enums end *********************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 检查硬件 I2C 句柄是否已初始化。
 *
 * @return en_core_iic_status_t : 就绪返回 CORE_IIC_OK，否则返回错误。
 * @note 硬件初始化由 CubeMX 的 MX_I2C1_Init 完成。
 */
en_core_iic_status_t core_iic_init(void);

/**
 * @brief 从指定寄存器同步读取连续数据。
 *
 * @param[in]  u8_device_address_7bit : 未左移的七位设备地址。
 * @param[in]  u8_register            : 设备内部八位寄存器地址。
 * @param[out] p_data                 : 接收缓冲区，由调用者保证有效。
 * @param[in]  u16_size               : 读取字节数。
 *
 * @return en_core_iic_status_t : 参数、接口或传输结果。
 */
en_core_iic_status_t core_iic_read(
    uint8_t        u8_device_address_7bit,
    uint8_t        u8_register,
    uint8_t       *p_data,
    uint16_t       u16_size);

/**
 * @brief 向指定寄存器同步写入连续数据。
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
    uint16_t       u16_size);

/**
 * @brief 在一次同步事务中读取高、低字节，失败时不更新输出。
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
    uint16_t *p_u16_value);

/**
 * @brief 按掩码修改双字节寄存器，保留其余位并等待至少 1ms。
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
    uint16_t u16_value);

/**
 * @brief 启动一次指定寄存器的 DMA 读。
 *
 * @param[in]  u8_device_address_7bit : 未左移的七位设备地址。
 * @param[in]  u8_register            : 设备内部八位寄存器地址。
 * @param[out] p_data                 : 接收缓冲区，由调用者保证有效。
 * @param[in]  u16_size               : 读取字节数。
 *
 * @return en_core_iic_status_t : 启动结果；CORE_IIC_OK 只表示已启动。
 * @note 完成事件由 HAL_I2C_MemRxCpltCallback 通知上层。
 */
en_core_iic_status_t core_iic_read_dma(
    uint8_t   u8_device_address_7bit,
    uint8_t   u8_register,
    uint8_t  *p_data,
    uint16_t  u16_size);

/**
 * @brief 中止当前 DMA 传输。
 *
 * @param[in] u8_device_address_7bit : 未左移的七位设备地址。
 *
 * @return en_core_iic_status_t : 中止请求结果。
 * @note 中止完成事件由 HAL_I2C_AbortCpltCallback 通知上层。
 */
en_core_iic_status_t core_iic_abort(uint8_t u8_device_address_7bit);
//************************* Function Prototypes end **************************//

#endif /* CORE_IIC_H */

//******************************* End of File ********************************//
