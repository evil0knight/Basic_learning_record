/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file i2c_port.c
 *
 * @par dependencies
 * - i2c_port.h
 * - stddef.h
 * - soft_i2c.h
 * - i2c_port_config.h
 *
 * @author YA
 *
 * @brief 检查参数、调用锁回调并分发硬件或软件事务。
 *
 * 处理流程：
 * 配置槽位后，通过 Port 接口完成参数检查、加锁、传输及解锁。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

//**************************** Includes begin ********************************//
#include "i2c_port.h"

#include <stddef.h>

#include "soft_i2c.h"
//****************************** Includes end ********************************//

//**************************** Typedefs begin ********************************//
/* 零值表示槽位尚未配置。 */
typedef enum
{
    CORE_I2C_TYPE_NONE = 0,
    CORE_I2C_TYPE_HARDWARE,
    CORE_I2C_TYPE_SOFTWARE
} en_core_i2c_type_t;

typedef struct
{
    en_core_i2c_type_t type;
    union
    {
        st_core_hardware_i2c_config_t  hardware;
        st_core_software_i2c_config_t software;
    } config;
} st_core_i2c_slot_t;

//****************************** Typedefs end ********************************//

//******************** Static Variables begin ********************************//
/* 配置表依赖上方私有类型，必须在此包含。 */
#include "i2c_port_config.h"
//********************** Static Variables end ********************************//

//******************** Static Functions begin ********************************//
/**
 * @brief 检查槽位范围、总线类型、板级资源和锁回调配对。
 *
 * 步骤：
 *  1. 检查槽位范围、总线类型、板级资源和锁回调配对。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
static en_core_i2c_status_t core_i2c_validate_port(
    core_i2c_port_id_t  port)
{
    const st_core_i2c_slot_t *slot;

    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    slot = &g_i2c_slots[port];
    if (slot->type == CORE_I2C_TYPE_NONE)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if ((slot->type != CORE_I2C_TYPE_HARDWARE) &&
        (slot->type != CORE_I2C_TYPE_SOFTWARE))
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if ((slot->type == CORE_I2C_TYPE_HARDWARE) &&
        ((slot->config.hardware.handle == NULL) ||
         ((slot->config.hardware.lock == NULL) !=
          (slot->config.hardware.unlock == NULL))))
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (slot->type == CORE_I2C_TYPE_SOFTWARE)
    {
        const st_core_software_i2c_config_t *config = &slot->config.software;

        if ((config->scl_port == NULL) || (config->scl_pin == 0U) ||
            (config->sda_port == NULL) || (config->sda_pin == 0U) ||
            (config->half_period_us == 0U) || (config->delay_us == NULL) ||
            ((config->lock == NULL) != (config->unlock == NULL)))
        {
            return CORE_I2C_ERR_NOT_CONFIGURED;
        }
    }

    return CORE_I2C_OK;
}

/**
 * @brief 检查槽位配置、七位地址、缓冲区和传输长度。
 *
 * 步骤：
 *  1. 检查槽位配置、七位地址、缓冲区和传输长度。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址，范围为 0x00～0x7F。
 * @param[in]  data                 : 发送缓冲区，不得为空。
 * @param[in]  size                 : 传输字节数，必须大于零。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
static en_core_i2c_status_t core_i2c_validate_transfer(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit,
    const void         *data,
    uint16_t            size)
{
    en_core_i2c_status_t status = core_i2c_validate_port(port);

    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if ((device_address_7bit > 0x7FU) || (data == NULL) || (size == 0U))
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    return CORE_I2C_OK;
}

/**
 * @brief 检查内部地址宽度是否为八位或十六位。
 *
 * 步骤：
 *  1. 检查内部地址宽度是否为八位或十六位。
 *
 * @param[in]  address_size         : 内部地址宽度，八位或十六位。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
static en_core_i2c_status_t core_i2c_validate_address_size(
    en_core_i2c_mem_addr_size_t  address_size)
{
    if ((address_size != CORE_I2C_MEM_ADDR_8BIT) &&
        (address_size != CORE_I2C_MEM_ADDR_16BIT))
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    return CORE_I2C_OK;
}

/**
 * @brief 将 HAL 返回值及应答错误转换为统一 I2C 状态。
 *
 * 步骤：
 *  1. 将 HAL 返回值及应答错误转换为统一 I2C 状态。
 *
 * @param[in]  handle               : HAL I2C 句柄，用于查询应答错误标志。
 * @param[in]  status               : 待转换的 HAL 返回状态。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
static en_core_i2c_status_t core_i2c_from_hal(
    I2C_HandleTypeDef *handle,
    HAL_StatusTypeDef  status)
{
    if (status == HAL_OK)
    {
        return CORE_I2C_OK;
    }
    if (status == HAL_BUSY)
    {
        return CORE_I2C_ERR_BUSY;
    }
    if (status == HAL_TIMEOUT)
    {
        return CORE_I2C_ERR_TIMEOUT;
    }
    if ((handle != NULL) &&
        ((HAL_I2C_GetError(handle) & HAL_I2C_ERROR_AF) != 0U))
    {
        return CORE_I2C_ERR_NACK;
    }

    return CORE_I2C_ERROR;
}

/**
 * @brief 根据槽位后端类型获取加锁回调。
 *
 * 步骤：
 *  1. 根据槽位后端类型获取加锁回调。
 *
 * @param[in]  slot                 : 已验证配置的槽位，不得为空。
 *
 * @return core_i2c_lock_fn_t : 对应锁回调，允许为空。
 */
static core_i2c_lock_fn_t core_i2c_get_lock(
    const st_core_i2c_slot_t *slot)
{
    return (slot->type == CORE_I2C_TYPE_HARDWARE)
               ? slot->config.hardware.lock
               : slot->config.software.lock;
}

/**
 * @brief 根据槽位后端类型获取解锁回调。
 *
 * 步骤：
 *  1. 根据槽位后端类型获取解锁回调。
 *
 * @param[in]  slot                 : 已验证配置的槽位，不得为空。
 *
 * @return core_i2c_unlock_fn_t : 对应锁回调，允许为空。
 */
static core_i2c_unlock_fn_t core_i2c_get_unlock(
    const st_core_i2c_slot_t *slot)
{
    return (slot->type == CORE_I2C_TYPE_HARDWARE)
               ? slot->config.hardware.unlock
               : slot->config.software.unlock;
}

/**
 * @brief 调用加锁回调，为空时跳过加锁，非零结果映射为总线忙。
 *
 * 步骤：
 *  1. 调用加锁回调，为空时跳过加锁，非零结果映射为总线忙。
 *
 * @param[in]  slot                 : 已验证配置的槽位，不得为空。
 * @param[in]  timeout_ms           : 锁回调及 HAL 操作的毫秒超时，非整个调用的总超时。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
static en_core_i2c_status_t core_i2c_lock(
    const st_core_i2c_slot_t *slot,
    uint32_t                  timeout_ms)
{
    core_i2c_lock_fn_t lock = core_i2c_get_lock(slot);

    if ((lock != NULL) && (lock(timeout_ms) != 0))
    {
        return CORE_I2C_ERR_BUSY;
    }

    return CORE_I2C_OK;
}

/**
 * @brief 调用解锁回调，为空时不执行操作。
 *
 * 步骤：
 *  1. 调用解锁回调，为空时不执行操作。
 *
 * @param[in]  slot                 : 已验证配置的槽位，不得为空。
 *
 * @return void : 无返回值。
 */
static void core_i2c_unlock(
    const st_core_i2c_slot_t *slot)
{
    core_i2c_unlock_fn_t unlock = core_i2c_get_unlock(slot);

    if (unlock != NULL)
    {
        unlock();
    }
}

//********************** Static Functions end ********************************//

//**************** Function Definitions begin ********************************//
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
    uint32_t            timeout_ms)
{
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_i2c_validate_transfer(port, device_address_7bit, data, size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];
    status = core_i2c_lock(slot, timeout_ms);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if (slot->type == CORE_I2C_TYPE_HARDWARE)
    {
        status = core_i2c_from_hal(
            slot->config.hardware.handle,
            HAL_I2C_Master_Transmit(slot->config.hardware.handle,
                                    (uint16_t)device_address_7bit << 1U,
                                    (uint8_t *)data,
                                    size,
                                    timeout_ms));
    }
    else
    {
        status = soft_i2c_write(&slot->config.software,
                                device_address_7bit,
                                data,
                                size);
    }

    core_i2c_unlock(slot);
    return status;
}

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
    uint32_t            timeout_ms)
{
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_i2c_validate_transfer(port, device_address_7bit, data, size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];
    status = core_i2c_lock(slot, timeout_ms);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if (slot->type == CORE_I2C_TYPE_HARDWARE)
    {
        status = core_i2c_from_hal(
            slot->config.hardware.handle,
            HAL_I2C_Master_Receive(slot->config.hardware.handle,
                                   (uint16_t)device_address_7bit << 1U,
                                   data,
                                   size,
                                   timeout_ms));
    }
    else
    {
        status = soft_i2c_read(&slot->config.software,
                               device_address_7bit,
                               data,
                               size);
    }

    core_i2c_unlock(slot);
    return status;
}

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
    uint32_t                     timeout_ms)
{
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_i2c_validate_transfer(port, device_address_7bit, data, size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }
    status = core_i2c_validate_address_size(address_size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];
    status = core_i2c_lock(slot, timeout_ms);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if (slot->type == CORE_I2C_TYPE_HARDWARE)
    {
        uint16_t hal_address_size = (address_size == CORE_I2C_MEM_ADDR_8BIT)
                                        ? I2C_MEMADD_SIZE_8BIT
                                        : I2C_MEMADD_SIZE_16BIT;
        status = core_i2c_from_hal(
            slot->config.hardware.handle,
            HAL_I2C_Mem_Write(slot->config.hardware.handle,
                              (uint16_t)device_address_7bit << 1U,
                              memory_address,
                              hal_address_size,
                              (uint8_t *)data,
                              size,
                              timeout_ms));
    }
    else
    {
        status = soft_i2c_mem_write(&slot->config.software,
                                    device_address_7bit,
                                    memory_address,
                                    address_size,
                                    data,
                                    size);
    }

    core_i2c_unlock(slot);
    return status;
}

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
    uint32_t                     timeout_ms)
{
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_i2c_validate_transfer(port, device_address_7bit, data, size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }
    status = core_i2c_validate_address_size(address_size);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];
    status = core_i2c_lock(slot, timeout_ms);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if (slot->type == CORE_I2C_TYPE_HARDWARE)
    {
        uint16_t hal_address_size = (address_size == CORE_I2C_MEM_ADDR_8BIT)
                                        ? I2C_MEMADD_SIZE_8BIT
                                        : I2C_MEMADD_SIZE_16BIT;
        status = core_i2c_from_hal(
            slot->config.hardware.handle,
            HAL_I2C_Mem_Read(slot->config.hardware.handle,
                             (uint16_t)device_address_7bit << 1U,
                             memory_address,
                             hal_address_size,
                             data,
                             size,
                             timeout_ms));
    }
    else
    {
        status = soft_i2c_mem_read(&slot->config.software,
                                   device_address_7bit,
                                   memory_address,
                                   address_size,
                                   data,
                                   size);
    }

    core_i2c_unlock(slot);
    return status;
}

/**
 * @brief 探测设备是否应答其地址。
 *
 * 步骤：
 *  1. 检查参数并调用加锁回调。
 *  2. 分发硬件 HAL 或软件 I2C 事务。
 *  3. 事务结束后释放已取得的锁，成功和失败路径一致。
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
    uint32_t            timeout_ms)
{
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_i2c_validate_port(port);
    if (status != CORE_I2C_OK)
    {
        return status;
    }
    if ((device_address_7bit > 0x7FU) || (trials == 0U))
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];
    status = core_i2c_lock(slot, timeout_ms);
    if (status != CORE_I2C_OK)
    {
        return status;
    }

    if (slot->type == CORE_I2C_TYPE_HARDWARE)
    {
        status = core_i2c_from_hal(
            slot->config.hardware.handle,
            HAL_I2C_IsDeviceReady(slot->config.hardware.handle,
                                  (uint16_t)device_address_7bit << 1U,
                                  trials,
                                  timeout_ms));
    }
    else
    {
        status = soft_i2c_is_ready(&slot->config.software,
                                   device_address_7bit,
                                   trials);
    }

    core_i2c_unlock(slot);
    return status;
}
//****************** Function Definitions end ********************************//

//******************************* End of File ********************************//
