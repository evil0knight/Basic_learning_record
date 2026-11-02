/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file i2c_port_os.h
 *
 * @par dependencies
 * - stdint.h
 * - ../../bus/inc/i2c_port_types.h
 * - osal.h
 *
 * @author YA
 *
 * @brief RTOS I2C Port 接口，无器件实例，锁与完成信号量按总线注册一次。
 *
 * Processing flow:
 *
 * core_i2c_init -> core_i2c_register_mutex/semaphore
 * -> 调用方每次传入设备地址 -> 同步接口内部加锁并等信号量
 * -> 异步接口启动 DMA 后在完成回调里通知调用方。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef I2C_PORT_OS_H
#define I2C_PORT_OS_H

//****************************** Includes begin ******************************//
#include <stdint.h>

#include "../../bus/inc/i2c_port_types.h"
#include "osal.h"
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//

//************************** 公共接口 begin **********************************//

/**
 * @brief 初始化 Bus 并注册 Port 的读、写、错误回调。
 */
en_core_i2c_status_t core_i2c_init(void);

/**
 * @brief 为指定总线注册互斥量，NULL 表示该总线不加锁。
 */
en_core_i2c_status_t core_i2c_register_mutex(
    core_i2c_port_id_t   port,
    osal_mutex_handle_t  mutex);

/**
 * @brief 为指定总线注册异步完成信号量，NULL 表示异步只靠回调通知。
 */
en_core_i2c_status_t core_i2c_register_semaphore(
    core_i2c_port_id_t  port,
    osal_sema_handle_t  sema);
//*************************** 公共接口 end ***********************************//

//************************** 死等接口 begin **********************************//

/**
 * @brief 普通 HAL 读寄存器，加总线锁后用轮询模式完成。
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
 * @brief 普通 HAL 写寄存器，加总线锁后用轮询模式完成。
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

/**
 * @brief DMA 读寄存器，本函数内等完成信号量。
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
 * @brief DMA 写寄存器，本函数内等完成信号量。
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
 * @brief 异步启动一次寄存器读，完成时给信号量并在中断上下文回调。
 */
en_core_i2c_status_t core_i2c_read_async(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size,
    core_i2c_done_callback_t     pf_done,
    void                        *p_user_data);

/**
 * @brief 异步启动一次寄存器写，完成时给信号量并在中断上下文回调。
 */
en_core_i2c_status_t core_i2c_write_async(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *p_data,
    uint16_t                     size,
    core_i2c_done_callback_t     pf_done,
    void                        *p_user_data);
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

#endif /* I2C_PORT_OS_H */

//******************************* End of File ********************************//
