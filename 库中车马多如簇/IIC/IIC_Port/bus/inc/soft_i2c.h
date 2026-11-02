/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file soft_i2c.h
 *
 * @par dependencies
 * - i2c_bus.h
 *
 * @author YA
 *
 * @brief 声明软件 I2C 内部事务接口。
 *
 * 处理流程：
 * 由 Bus 在持锁上下文中调用，按位翻转完成软件 I2C 收发。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

#ifndef SOFT_I2C_H
#define SOFT_I2C_H

//**************************** Includes begin ********************************//
#include "i2c_bus.h"
//****************************** Includes end ********************************//

//***************** Function Prototypes begin ********************************//
/**
 * @brief 向设备写入连续数据。
 */
en_core_i2c_status_t soft_i2c_write(
    const st_core_software_i2c_config_t *config,
    uint8_t                              device_address_7bit,
    const uint8_t                       *data,
    uint16_t                             size);
/**
 * @brief 从设备读取连续数据。
 */
en_core_i2c_status_t soft_i2c_read(
    const st_core_software_i2c_config_t *config,
    uint8_t                              device_address_7bit,
    uint8_t                             *data,
    uint16_t                             size);
/**
 * @brief 向设备指定内部地址写入数据。
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
 */
en_core_i2c_status_t soft_i2c_is_ready(
    const st_core_software_i2c_config_t *config,
    uint8_t                              device_address_7bit,
    uint32_t                             trials);

//******************* Function Prototypes end ********************************//

#endif /* SOFT_I2C_H */

//******************************* End of File ********************************//
