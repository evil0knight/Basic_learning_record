/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file soft_i2c.h
 *
 * @par dependencies
 * - i2c_port.h
 *
 * @author YA
 *
 * @brief 声明软件 I2C 内部事务接口。
 *
 * 处理流程：
 * 配置槽位后，通过 Port 接口完成参数检查、加锁、传输及解锁。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

#ifndef SOFT_I2C_H
#define SOFT_I2C_H

//**************************** Includes begin ********************************//
#include "i2c_port.h"
//****************************** Includes end ********************************//

//***************** Function Prototypes begin ********************************//
/**
 * @brief 向设备写入连续数据。
 *
 * 步骤：
 *  1. 产生起始条件，发送地址并执行数据传输或探测。
 *  2. 事务结束时尝试 STOP；停止失败时优先返回停止错误。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址，范围为 0x00～0x7F。
 * @param[in]  data                 : 发送缓冲区，不得为空。
 * @param[in]  size                 : 传输字节数，必须大于零。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
en_core_i2c_status_t soft_i2c_write(
    const st_core_software_i2c_config_t *config,
    uint8_t                              device_address_7bit,
    const uint8_t                       *data,
    uint16_t                             size);
/**
 * @brief 从设备读取连续数据。
 *
 * 步骤：
 *  1. 产生起始条件，发送地址并执行数据传输或探测。
 *  2. 事务结束时尝试 STOP；停止失败时优先返回停止错误。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址，范围为 0x00～0x7F。
 * @param[out] data                 : 接收缓冲区，不得为空。
 * @param[in]  size                 : 传输字节数，必须大于零。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
en_core_i2c_status_t soft_i2c_read(
    const st_core_software_i2c_config_t *config,
    uint8_t                              device_address_7bit,
    uint8_t                             *data,
    uint16_t                             size);
/**
 * @brief 向设备指定内部地址写入数据。
 *
 * 步骤：
 *  1. 产生起始条件，发送地址并执行数据传输或探测。
 *  2. 事务结束时尝试 STOP；停止失败时优先返回停止错误。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址，范围为 0x00～0x7F。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度，八位或十六位。
 * @param[in]  data                 : 发送缓冲区，不得为空。
 * @param[in]  size                 : 传输字节数，必须大于零。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
en_core_i2c_status_t soft_i2c_mem_write(
    const st_core_software_i2c_config_t *config,
    uint8_t                              device_address_7bit,
    uint16_t                             memory_address,
    en_core_i2c_mem_addr_size_t          address_size,
    const uint8_t                       *data,
    uint16_t                             size);
/**
 * @brief 从设备指定内部地址读取数据。
 *
 * 步骤：
 *  1. 产生起始条件，发送地址并执行数据传输或探测。
 *  2. 事务结束时尝试 STOP；停止失败时优先返回停止错误。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址，范围为 0x00～0x7F。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度，八位或十六位。
 * @param[out] data                 : 接收缓冲区，不得为空。
 * @param[in]  size                 : 传输字节数，必须大于零。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
en_core_i2c_status_t soft_i2c_mem_read(
    const st_core_software_i2c_config_t *config,
    uint8_t                              device_address_7bit,
    uint16_t                             memory_address,
    en_core_i2c_mem_addr_size_t          address_size,
    uint8_t                             *data,
    uint16_t                             size);
/**
 * @brief 探测设备是否应答其地址。
 *
 * 步骤：
 *  1. 产生起始条件，发送地址并执行数据传输或探测。
 *  2. 事务结束时尝试 STOP；停止失败时优先返回停止错误。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址，范围为 0x00～0x7F。
 * @param[in]  trials               : 最大探测次数，必须大于零。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
en_core_i2c_status_t soft_i2c_is_ready(
    const st_core_software_i2c_config_t *config,
    uint8_t                              device_address_7bit,
    uint32_t                             trials);

//******************* Function Prototypes end ********************************//

#endif /* SOFT_I2C_H */

//******************************* End of File ********************************//
