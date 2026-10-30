/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file i2c_port_config.h
 *
 * @par dependencies
 * - i2c_port.c 中定义的私有槽位类型
 *
 * @author YA
 *
 * @brief 配置逻辑 I2C 槽位及板级资源。
 *
 * 处理流程：
 * 配置槽位后，通过 Port 接口完成参数检查、加锁、传输及解锁。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

#ifndef I2C_PORT_CONFIG_H
#define I2C_PORT_CONFIG_H

//******************** Static Variables begin ********************************//
/* 
 * #include "i2c.h"
 *
 * static st_core_i2c_slot_t g_i2c_slots[CORE_I2C_PORT_MAX] = {
 *     [0U] = {
 *         .type = CORE_I2C_TYPE_HARDWARE,
 *         .config.hardware = {
 *             .handle = &hi2c1,
 *             .lock   = NULL,
 *             .unlock = NULL
 *         }
 *     }
 * };
 *
 *
 * extern void board_delay_us(uint32_t delay_us);
 *
 * static st_core_i2c_slot_t g_i2c_slots[CORE_I2C_PORT_MAX] = {
 *     [1U] = {
 *         .type = CORE_I2C_TYPE_SOFTWARE,
 *         .config.software = {
 *             .scl_port       = GPIOB,
 *             .scl_pin        = GPIO_PIN_8,
 *             .sda_port       = GPIOB,
 *             .sda_pin        = GPIO_PIN_9,
 *             .half_period_us = 5U,
 *             .delay_us       = board_delay_us,
 *             .lock           = NULL,
 *             .unlock         = NULL
 *         }
 *     }
 * };
 *
 */
static st_core_i2c_slot_t g_i2c_slots[CORE_I2C_PORT_MAX] = {0};

//********************** Static Variables end ********************************//

#endif /* I2C_PORT_CONFIG_H */

//******************************* End of File ********************************//
