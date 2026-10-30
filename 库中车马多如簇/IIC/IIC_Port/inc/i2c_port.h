/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file i2c_port.h
 *
 * @par dependencies
 * - stdint.h
 * - main.h
 *
 * @author YA
 *
 * @brief 提供统一阻塞式 I2C 接口与板级资源类型。
 *
 * 处理流程：
 * 配置槽位后，通过 Port 接口完成参数检查、加锁、传输及解锁。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

#ifndef I2C_PORT_H
#define I2C_PORT_H

//**************************** Includes begin ********************************//
#include <stdint.h>

#include "main.h"
//****************************** Includes end ********************************//

//**************************** Typedefs begin ********************************//
/* 统一 I2C 返回状态。 */
typedef enum
{
    CORE_I2C_OK = 0,
    CORE_I2C_ERROR,
    CORE_I2C_ERR_ARGUMENT,
    CORE_I2C_ERR_TIMEOUT,
    CORE_I2C_ERR_BUSY,
    CORE_I2C_ERR_NOT_CONFIGURED,
    CORE_I2C_ERR_NACK
} en_core_i2c_status_t;

typedef uint8_t core_i2c_port_id_t;

#define CORE_I2C_PORT_MAX 8U

typedef enum
{
    CORE_I2C_MEM_ADDR_8BIT = 1,
    CORE_I2C_MEM_ADDR_16BIT = 2
} en_core_i2c_mem_addr_size_t;

/* 加锁回调返回 0 表示成功，非零表示失败，超时单位为毫秒。
 * lock/unlock 必须成对配置或同时为空；为空时不提供互斥保护。
 * 多任务访问同一物理总线时，各槽位须通过回调使用同一个 mutex。
 */
typedef int (*core_i2c_lock_fn_t)(uint32_t timeout_ms);
typedef void (*core_i2c_unlock_fn_t)(void);

typedef struct
{
    I2C_HandleTypeDef     *handle;
    core_i2c_lock_fn_t     lock;
    core_i2c_unlock_fn_t   unlock;
} st_core_hardware_i2c_config_t;

typedef struct
{
    GPIO_TypeDef         *scl_port;
    uint16_t              scl_pin;
    GPIO_TypeDef         *sda_port;
    uint16_t              sda_pin;
    uint32_t              half_period_us;
    void                (*delay_us)(uint32_t delay_us);
    core_i2c_lock_fn_t     lock;
    core_i2c_unlock_fn_t   unlock;
} st_core_software_i2c_config_t;

//****************************** Typedefs end ********************************//

//***************** Function Prototypes begin ********************************//
/**
 * @brief 向设备写入连续数据。
 *
 * 步骤：
 *  1. 检查参数并调用加锁回调。
 *  2. 分发硬件 HAL 或软件 I2C 事务。
 *  3. 事务结束后释放已取得的锁，成功和失败路径一致。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址，范围为 0x00～0x7F。
 * @param[in]  data                 : 发送缓冲区，不得为空。
 * @param[in]  size                 : 传输字节数，必须大于零。
 * @param[in]  timeout_ms           : 锁回调及 HAL 操作的毫秒超时，非整个调用的总超时。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 本模块不创建 mutex；lock/unlock 同时为空时跳过加锁。
 *       锁等待和 HAL 分别计时；软件时序不使用 timeout_ms。
 */
en_core_i2c_status_t core_i2c_write(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit,
    const uint8_t      *data,
    uint16_t            size,
    uint32_t            timeout_ms);

/**
 * @brief 从设备读取连续数据。
 *
 * 步骤：
 *  1. 检查参数并调用加锁回调。
 *  2. 分发硬件 HAL 或软件 I2C 事务。
 *  3. 事务结束后释放已取得的锁，成功和失败路径一致。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址，范围为 0x00～0x7F。
 * @param[out] data                 : 接收缓冲区，不得为空。
 * @param[in]  size                 : 传输字节数，必须大于零。
 * @param[in]  timeout_ms           : 锁回调及 HAL 操作的毫秒超时，非整个调用的总超时。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 本模块不创建 mutex；lock/unlock 同时为空时跳过加锁。
 *       锁等待和 HAL 分别计时；软件时序不使用 timeout_ms。
 */
en_core_i2c_status_t core_i2c_read(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit,
    uint8_t            *data,
    uint16_t            size,
    uint32_t            timeout_ms);

/**
 * @brief 向设备指定内部地址写入数据。
 *
 * 步骤：
 *  1. 检查参数并调用加锁回调。
 *  2. 分发硬件 HAL 或软件 I2C 事务。
 *  3. 事务结束后释放已取得的锁，成功和失败路径一致。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址，范围为 0x00～0x7F。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度，八位或十六位。
 * @param[in]  data                 : 发送缓冲区，不得为空。
 * @param[in]  size                 : 传输字节数，必须大于零。
 * @param[in]  timeout_ms           : 锁回调及 HAL 操作的毫秒超时，非整个调用的总超时。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 本模块不创建 mutex；lock/unlock 同时为空时跳过加锁。
 *       锁等待和 HAL 分别计时；软件时序不使用 timeout_ms。
 */
en_core_i2c_status_t core_i2c_mem_write(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *data,
    uint16_t                     size,
    uint32_t                     timeout_ms);

/**
 * @brief 从设备指定内部地址读取数据。
 *
 * 步骤：
 *  1. 检查参数并调用加锁回调。
 *  2. 分发硬件 HAL 或软件 I2C 事务。
 *  3. 事务结束后释放已取得的锁，成功和失败路径一致。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址，范围为 0x00～0x7F。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度，八位或十六位。
 * @param[out] data                 : 接收缓冲区，不得为空。
 * @param[in]  size                 : 传输字节数，必须大于零。
 * @param[in]  timeout_ms           : 锁回调及 HAL 操作的毫秒超时，非整个调用的总超时。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 本模块不创建 mutex；lock/unlock 同时为空时跳过加锁。
 *       锁等待和 HAL 分别计时；软件时序不使用 timeout_ms。
 */
en_core_i2c_status_t core_i2c_mem_read(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *data,
    uint16_t                     size,
    uint32_t                     timeout_ms);

/**
 * @brief 探测设备是否应答其地址。
 *
 * 步骤：
 *  1. 检查参数并调用加锁回调
 *  2. 分发硬件 HAL 或软件 I2C 事务
 *  3. 事务结束后释放已取得的锁，成功和失败路径一致
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址，范围为 0x00～0x7F。
 * @param[in]  trials               : 最大探测次数，必须大于零。
 * @param[in]  timeout_ms           : 锁回调及 HAL 操作的毫秒超时，非整个调用的总超时。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 本模块不创建 mutex；lock/unlock 同时为空时跳过加锁。
 *       锁等待和 HAL 分别计时；软件时序不使用 timeout_ms。
 */
en_core_i2c_status_t core_i2c_is_ready(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit,
    uint32_t            trials,
    uint32_t            timeout_ms);

//******************* Function Prototypes end ********************************//

#endif /* I2C_PORT_H */

//******************************* End of File ********************************//
