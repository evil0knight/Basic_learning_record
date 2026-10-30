/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file soft_i2c.c
 *
 * @par dependencies
 * - soft_i2c.h
 * - gpio_port.h
 *
 * @author YA
 *
 * @brief 通过 GPIO 生成软件 I2C 时序。
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
#include "soft_i2c.h"

#include "gpio_port.h"
//****************************** Includes end ********************************//

//******************** Static Functions begin ********************************//
/**
 * @brief 调用微秒延时回调等待一个软件 I2C 半周期。
 *
 * 步骤：
 *  1. 调用微秒延时回调等待一个软件 I2C 半周期。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 *
 * @return void : 无返回值。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
static void soft_i2c_delay(
    const st_core_software_i2c_config_t *config)
{
    config->delay_us(config->half_period_us);
}

/**
 * @brief 通过直接 GPIO 接口设置引脚电平。
 *
 * 步骤：
 *  1. 通过直接 GPIO 接口设置引脚电平。
 *
 * @param[in]  port                 : GPIO 端口。
 * @param[in]  pin                  : GPIO 引脚掩码。
 * @param[in]  state                : 待输出的引脚电平。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
static en_core_i2c_status_t soft_i2c_set(
    GPIO_TypeDef  *port,
    uint16_t       pin,
    GPIO_PinState  state)
{
    return (core_gpio_write_pin_direct(port, pin, state) == CORE_GPIO_OK)
               ? CORE_I2C_OK
               : CORE_I2C_ERROR;
}

/**
 * @brief 通过直接 GPIO 接口读取引脚电平。
 *
 * 步骤：
 *  1. 通过直接 GPIO 接口读取引脚电平。
 *
 * @param[in]  port                 : GPIO 端口。
 * @param[in]  pin                  : GPIO 引脚掩码。
 * @param[out] state                : 读取到的引脚电平，不得为空。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
static en_core_i2c_status_t soft_i2c_get(
    GPIO_TypeDef  *port,
    uint16_t       pin,
    GPIO_PinState *state)
{
    return (core_gpio_read_pin_direct(port, pin, state) == CORE_GPIO_OK)
               ? CORE_I2C_OK
               : CORE_I2C_ERROR;
}

/**
 * @brief 释放 SDA 和 SCL 后拉低 SDA，再拉低 SCL，产生起始条件。
 *
 * 步骤：
 *  1. 释放 SDA 和 SCL 后拉低 SDA，再拉低 SCL，产生起始条件。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
static en_core_i2c_status_t soft_i2c_start(
    const st_core_software_i2c_config_t *config)
{
    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_SET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_RESET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    return soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_RESET);
}

/**
 * @brief 拉低 SDA 后释放 SCL，再释放 SDA，产生停止条件。
 *
 * 步骤：
 *  1. 拉低 SDA 后释放 SCL，再释放 SDA，产生停止条件。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
static en_core_i2c_status_t soft_i2c_stop(
    const st_core_software_i2c_config_t *config)
{
    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_RESET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_SET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    return CORE_I2C_OK;
}

/**
 * @brief 尝试发送停止条件，停止失败时优先返回停止错误。
 *
 * 步骤：
 *  1. 尝试发送停止条件，停止失败时优先返回停止错误。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 * @param[in]  status               : 停止前的事务状态。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
static en_core_i2c_status_t soft_i2c_finish(
    const st_core_software_i2c_config_t *config,
    en_core_i2c_status_t                 status)
{
    en_core_i2c_status_t stop_status = soft_i2c_stop(config);

    return (stop_status == CORE_I2C_OK) ? status : stop_status;
}

/**
 * @brief 从最高位发送一个字节并读取从机应答。
 *
 * 步骤：
 *  1. 从最高位发送一个字节并读取从机应答。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 * @param[in]  data                 : 待发送的一个字节。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
static en_core_i2c_status_t soft_i2c_send_byte(
    const st_core_software_i2c_config_t *config,
    uint8_t                              data)
{
    uint8_t       bit;
    GPIO_PinState ack_state;

    for (bit = 0U; bit < 8U; bit++)
    {
        GPIO_PinState state = ((data & 0x80U) != 0U)
                                  ? GPIO_PIN_SET
                                  : GPIO_PIN_RESET;

        if (soft_i2c_set(config->sda_port, config->sda_pin, state)
        != CORE_I2C_OK)
            return CORE_I2C_ERROR;
        data <<= 1U;
        soft_i2c_delay(config);
        if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET)
        != CORE_I2C_OK)
            return CORE_I2C_ERROR;
        soft_i2c_delay(config);
        if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_RESET)
        != CORE_I2C_OK)
            return CORE_I2C_ERROR;
    }

    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_SET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_get(config->sda_port, config->sda_pin, &ack_state)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_RESET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;

    return (ack_state == GPIO_PIN_RESET) ? CORE_I2C_OK : CORE_I2C_ERR_NACK;
}

/**
 * @brief 逐位读取一个字节，并根据参数发送 ACK 或 NACK。
 *
 * 步骤：
 *  1. 逐位读取一个字节，并根据参数发送 ACK 或 NACK。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 * @param[out] data                 : 接收缓冲区，不得为空。
 * @param[in]  acknowledge          : 非零发送 ACK，零发送 NACK。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
static en_core_i2c_status_t soft_i2c_receive_byte(
    const st_core_software_i2c_config_t *config,
    uint8_t                             *data,
    int                                  acknowledge)
{
    uint8_t bit;

    *data = 0U;
    if (soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_SET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;

    for (bit = 0U; bit < 8U; bit++)
    {
        GPIO_PinState state;

        *data <<= 1U;
        soft_i2c_delay(config);
        if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET)
        != CORE_I2C_OK)
            return CORE_I2C_ERROR;
        soft_i2c_delay(config);
        if (soft_i2c_get(config->sda_port, config->sda_pin, &state)
        != CORE_I2C_OK)
            return CORE_I2C_ERROR;
        if (state == GPIO_PIN_SET)
        {
            *data |= 1U;
        }
        if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_RESET)
        != CORE_I2C_OK)
            return CORE_I2C_ERROR;
    }

    if (soft_i2c_set(config->sda_port,
                     config->sda_pin,
                     acknowledge ? GPIO_PIN_RESET : GPIO_PIN_SET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_SET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    soft_i2c_delay(config);
    if (soft_i2c_set(config->scl_port, config->scl_pin, GPIO_PIN_RESET)
        != CORE_I2C_OK)
        return CORE_I2C_ERROR;
    return soft_i2c_set(config->sda_port, config->sda_pin, GPIO_PIN_SET);
}

/**
 * @brief 发送内部地址，十六位地址先发送高字节。
 *
 * 步骤：
 *  1. 发送内部地址，十六位地址先发送高字节。
 *
 * @param[in]  config               : 已由 Port 层验证的软件总线配置，不得为空。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度，八位或十六位。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 *
 * @note 内部接口由 Port 层负责参数检查及完整事务的加锁。
 */
static en_core_i2c_status_t soft_i2c_send_memory_address(
    const st_core_software_i2c_config_t *config,
    uint16_t                             memory_address,
    en_core_i2c_mem_addr_size_t          address_size)
{
    en_core_i2c_status_t status;

    if (address_size == CORE_I2C_MEM_ADDR_16BIT)
    {
        status = soft_i2c_send_byte(config, (uint8_t)(memory_address >> 8U));
        if (status != CORE_I2C_OK)
        {
            return status;
        }
    }

    return soft_i2c_send_byte(config, (uint8_t)memory_address);
}

//********************** Static Functions end ********************************//

//**************** Function Definitions begin ********************************//
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
    uint16_t                             size)
{
    uint16_t index;
    en_core_i2c_status_t status = soft_i2c_start(config);

    if (status == CORE_I2C_OK)
        status = soft_i2c_send_byte(
            config, (uint8_t)(device_address_7bit << 1U));
    for (index = 0U; (index < size) && (status == CORE_I2C_OK); index++)
        status = soft_i2c_send_byte(config, data[index]);

    return soft_i2c_finish(config, status);
}

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
    uint16_t                             size)
{
    uint16_t index;
    en_core_i2c_status_t status = soft_i2c_start(config);

    if (status == CORE_I2C_OK)
        status = soft_i2c_send_byte(config,
            (uint8_t)((device_address_7bit << 1U) | 1U));
    for (index = 0U; (index < size) && (status == CORE_I2C_OK); index++)
        status = soft_i2c_receive_byte(config, &data[index], index + 1U < size);

    return soft_i2c_finish(config, status);
}

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
    uint16_t                             size)
{
    uint16_t index;
    en_core_i2c_status_t status = soft_i2c_start(config);

    if (status == CORE_I2C_OK)
        status = soft_i2c_send_byte(
            config, (uint8_t)(device_address_7bit << 1U));
    if (status == CORE_I2C_OK)
        status = soft_i2c_send_memory_address(
            config, memory_address, address_size);
    for (index = 0U; (index < size) && (status == CORE_I2C_OK); index++)
        status = soft_i2c_send_byte(config, data[index]);

    return soft_i2c_finish(config, status);
}

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
    uint16_t                             size)
{
    uint16_t index;
    en_core_i2c_status_t status = soft_i2c_start(config);

    if (status == CORE_I2C_OK)
        status = soft_i2c_send_byte(
            config, (uint8_t)(device_address_7bit << 1U));
    if (status == CORE_I2C_OK)
        status = soft_i2c_send_memory_address(
            config, memory_address, address_size);
    if (status == CORE_I2C_OK)
        status = soft_i2c_start(config);
    if (status == CORE_I2C_OK)
        status = soft_i2c_send_byte(config,
            (uint8_t)((device_address_7bit << 1U) | 1U));
    for (index = 0U; (index < size) && (status == CORE_I2C_OK); index++)
        status = soft_i2c_receive_byte(config, &data[index], index + 1U < size);

    return soft_i2c_finish(config, status);
}

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
    uint32_t                             trials)
{
    uint32_t trial;

    for (trial = 0U; trial < trials; trial++)
    {
        en_core_i2c_status_t status = soft_i2c_start(config);

        if (status == CORE_I2C_OK)
            status = soft_i2c_send_byte(
                config, (uint8_t)(device_address_7bit << 1U));
        status = soft_i2c_finish(config, status);
        if (status == CORE_I2C_OK)
        {
            return CORE_I2C_OK;
        }
        if (status != CORE_I2C_ERR_NACK)
        {
            return status;
        }
    }

    return CORE_I2C_ERR_NACK;
}
//****************** Function Definitions end ********************************//

//******************************* End of File ********************************//
