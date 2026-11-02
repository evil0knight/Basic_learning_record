/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file i2c_port_bare.h
 *
 * @par dependencies
 * - stdint.h
 * - ../../bus/inc/i2c_port_types.h
 *
 * @author YA
 *
 * @brief 裸机 I2C Port 接口，请求队列加硬件超时定时器，全事件驱动。
 *
 * Processing flow:
 *
 * submit 入队并启动 DMA -> 启动超时定时器
 * -> DMA 完成中断或定时器中断先到，统一进分发函数回调提交方
 * -> 分发完自动启动下一个请求，主循环不需要 poll。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef I2C_PORT_BARE_H
#define I2C_PORT_BARE_H

//****************************** Includes begin ******************************//
#include <stdint.h>

#include "../../bus/inc/i2c_port_types.h"
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//

//************************** 公共接口 begin **********************************//

/**
 * @brief 初始化 Bus 并注册 Port 的读、写、错误回调。
 */
en_core_i2c_status_t core_i2c_init(void);
//*************************** 公共接口 end ***********************************//

//************************** 死等接口 begin **********************************//

/**
 * @brief 普通 HAL 读寄存器，CPU 在 HAL 内部轮询，返回即完成。
 */
en_core_i2c_status_t core_i2c_mem_read(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms);

/**
 * @brief 普通 HAL 写寄存器，CPU 在 HAL 内部轮询，返回即完成。
 */
en_core_i2c_status_t core_i2c_mem_write(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms);
//*************************** 死等接口 end ***********************************//

//************************** 同步接口 begin **********************************//

/* 非阻塞轮询：每次调用都会立刻返回。
 * 首次调用提交一笔请求并返回 CORE_I2C_PENDING；
 * 之后周期性调用，未完成时返回 CORE_I2C_PENDING；
 * 完成时返回本次传输的结果，缓冲区已就绪。
 * 超时按首次提交到当前的毫秒差判定，超时会自动 abort。
 */

/**
 * @brief DMA 读寄存器，提交后立刻返回，下次调用收结果。
 */
en_core_i2c_status_t core_i2c_mem_read_sync(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms);

/**
 * @brief DMA 写寄存器，提交后立刻返回，下次调用收结果。
 */
en_core_i2c_status_t core_i2c_mem_write_sync(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms);
//*************************** 同步接口 end ***********************************//

//************************** 异步接口 begin **********************************//

/**
 * @brief 把一次请求放进指定总线的队列，入队成功后会尝试立刻启动。
 */
en_core_i2c_status_t core_i2c_submit(core_i2c_request_t *p_request);
//*************************** 异步接口 end ***********************************//

//************************** 辅助接口 begin **********************************//

/**
 * @brief 中止指定总线上的 DMA 传输。
 */
en_core_i2c_status_t core_i2c_abort(core_i2c_port_id_t port);

/**
 * @brief 读取最近一次传输的 HAL 原始错误码，0 表示无错误。
 */
uint32_t core_i2c_get_error_code(core_i2c_port_id_t port);
//*************************** 辅助接口 end ***********************************//

//************************* Function Prototypes end **************************//

#endif /* I2C_PORT_BARE_H */

//******************************* End of File ********************************//
