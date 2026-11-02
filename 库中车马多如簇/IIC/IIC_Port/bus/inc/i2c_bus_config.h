/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file i2c_bus_config.h
 *
 * @par dependencies
 * - i2c.h
 * - i2c_bus.c 中定义的私有槽位类型
 *
 * @author YA
 *
 * @brief 配置逻辑 I2C 槽位及板级资源。
 *
 * 处理流程：
 * 配置槽位后，通过 Bus 接口完成参数检查、传输及完成分发。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

#ifndef I2C_BUS_CONFIG_H
#define I2C_BUS_CONFIG_H

//******************** Static Variables begin ********************************//
/*
 * 硬件 I2C 槽位：每条物理总线占一个槽位。
 *
 * 说明：
 * 1. 异步 DMA 需要 CubeMX 打开对应 I2C 的 RX/TX DMA 和 NVIC。
 * 2. Bus 不挂锁、不挂 OS 资源；锁和信号量由 Port 负责。
 * 3. 软件 I2C 槽位同样支持，但不支持 DMA，调用 DMA 接口返回
 *    CORE_I2C_ERR_UNSUPPORTED。
 * 4. CORE_I2C_HW_ENABLE 定义为 0 时，槽位必须改成软件配置，
 *    否则 st_core_hardware_i2c_config_t 不存在会编译失败。
 */
#include "i2c.h"

static st_core_i2c_slot_t g_i2c_slots[CORE_I2C_PORT_MAX] = {
    [CORE_I2C_PORT_ENCODER1] = {
         .type = CORE_I2C_TYPE_HARDWARE,
         .config.hardware = {
             .handle = &hi2c1
         }
    }
};

//********************** Static Variables end ********************************//

#endif /* I2C_BUS_CONFIG_H */

//******************************* End of File ********************************//
