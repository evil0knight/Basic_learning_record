/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file i2c_bus.c
 *
 * @par dependencies
 * - i2c_bus.h
 * - stddef.h
 * - soft_i2c.h
 * - i2c_bus_config.h
 *
 * @author YA
 *
 * @brief 检查参数并分发硬件或软件 I2C 事务。
 *
 * 处理流程：
 * 配置槽位后，Bus 完成参数检查、传输及完成分发；
 * 锁、超时和同步等待由 Port 层负责。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

//**************************** Includes begin ********************************//
#include "../inc/i2c_bus.h"
#include <stddef.h>
#include "../inc/soft_i2c.h"
//****************************** Includes end ********************************//

//**************************** Typedefs begin ********************************//
/* 零值表示槽位尚未配置。 */
typedef enum
{
    CORE_I2C_TYPE_NONE = 0,
    CORE_I2C_TYPE_HARDWARE,
    CORE_I2C_TYPE_SOFTWARE
} en_core_i2c_type_t;

/* 当前 DMA 传输方向。 */
typedef enum
{
    CORE_I2C_DMA_KIND_NONE = 0,
    CORE_I2C_DMA_KIND_READ,
    CORE_I2C_DMA_KIND_WRITE
} en_core_i2c_dma_kind_t;

/* 一次 DMA 传输的最小上下文，只给完成回调使用。 */
typedef struct
{
    uint8_t                *p_buffer;
    uint16_t                size;
    uint8_t                 device_address_7bit;
    en_core_i2c_dma_kind_t  kind;
} st_core_i2c_dma_ctx_t;

typedef struct
{
    en_core_i2c_type_t    type;      // 硬件、软件或未配置
    volatile uint8_t      busy;      // 总线占用标志
    i2c_rx_callback_t     pf_rx;     // 读完成回调
    i2c_tx_callback_t     pf_tx;     // 写完成回调
    i2c_error_callback_t  pf_error;  // 错误回调
    union
    {
#if (CORE_I2C_HW_ENABLE != 0)
        st_core_hardware_i2c_config_t  hardware;
#endif
        st_core_software_i2c_config_t software;
    } config;
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_dma_ctx_t dma;
#endif
} st_core_i2c_slot_t;

//****************************** Typedefs end ********************************//

//******************** Static Variables begin ********************************//
/* 配置表依赖上方私有类型，必须在此包含。 */
#include "../inc/i2c_bus_config.h"
//********************** Static Variables end ********************************//

//******************** Static Functions begin ********************************//
/* 直接层：只面向 HAL，供上面的逻辑层调用，不对外公开。 */
static en_core_i2c_status_t i2c_bus_write_direct(
    I2C_HandleTypeDef *p_handle,
    uint8_t            device_address_7bit,
    const uint8_t     *data,
    uint16_t           size,
    uint32_t           timeout_ms);

static en_core_i2c_status_t i2c_bus_read_direct(
    I2C_HandleTypeDef *p_handle,
    uint8_t            device_address_7bit,
    uint8_t           *data,
    uint16_t           size,
    uint32_t           timeout_ms);

static en_core_i2c_status_t i2c_bus_mem_write_direct(
    I2C_HandleTypeDef           *p_handle,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *data,
    uint16_t                     size,
    uint32_t                     timeout_ms);

static en_core_i2c_status_t i2c_bus_mem_read_direct(
    I2C_HandleTypeDef           *p_handle,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *data,
    uint16_t                     size,
    uint32_t                     timeout_ms);

static en_core_i2c_status_t i2c_bus_is_ready_direct(
    I2C_HandleTypeDef *p_handle,
    uint8_t            device_address_7bit,
    uint32_t           trials,
    uint32_t           timeout_ms);

static en_core_i2c_status_t i2c_bus_mem_read_dma_direct(
    I2C_HandleTypeDef           *p_handle,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size);

static en_core_i2c_status_t i2c_bus_mem_write_dma_direct(
    I2C_HandleTypeDef           *p_handle,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *p_data,
    uint16_t                     size);

static en_core_i2c_status_t i2c_bus_abort_dma_direct(
    I2C_HandleTypeDef *p_handle);

/**
 * @brief 检查槽位范围、总线类型和板级资源。
 *
 * 步骤：
 *  1. 检查槽位范围和总线类型。
 *  2. 检查硬件句柄或软件引脚资源。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
static en_core_i2c_status_t i2c_bus_validate_port(
    core_i2c_port_id_t  port)
{
    const st_core_i2c_slot_t *slot;

    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    slot = &g_i2c_slots[port];
    if (CORE_I2C_TYPE_NONE == slot->type)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if ((CORE_I2C_TYPE_HARDWARE != slot->type) &&
        (CORE_I2C_TYPE_SOFTWARE != slot->type))
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

#if (CORE_I2C_HW_ENABLE != 0)
    if (CORE_I2C_TYPE_HARDWARE == slot->type)
    {
        if (NULL == slot->config.hardware.handle)
        {
            return CORE_I2C_ERR_NOT_CONFIGURED;
        }
    }
#else
    if (CORE_I2C_TYPE_HARDWARE == slot->type)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }
#endif

    if (CORE_I2C_TYPE_SOFTWARE == slot->type)
    {
        const st_core_software_i2c_config_t *config = &slot->config.software;

        if ((NULL == config->scl_port) || (0U == config->scl_pin) ||
            (NULL == config->sda_port) || (0U == config->sda_pin) ||
            (0U == config->half_period_us) || (NULL == config->delay_us))
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
 *  1. 检查槽位配置。
 *  2. 检查七位地址范围、缓冲区和长度。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  data                 : 发送或接收缓冲区，不得为空。
 * @param[in]  size                 : 传输字节数，必须大于零。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
static en_core_i2c_status_t i2c_bus_validate_transfer(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit,
    const void         *data,
    uint16_t            size)
{
    en_core_i2c_status_t status = i2c_bus_validate_port(port);

    if (CORE_I2C_OK != status)
    {
        return status;
    }

    if ((device_address_7bit > 0x7FU) || (NULL == data) || (0U == size))
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
 * @param[in]  address_size         : 内部地址宽度。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
static en_core_i2c_status_t i2c_bus_validate_address_size(
    en_core_i2c_mem_addr_size_t  address_size)
{
    if ((CORE_I2C_MEM_ADDR_8BIT != address_size) &&
        (CORE_I2C_MEM_ADDR_16BIT != address_size))
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    return CORE_I2C_OK;
}

#if (CORE_I2C_HW_ENABLE != 0)
/**
 * @brief 将 HAL 返回值及应答错误转换为统一 I2C 状态。
 *
 * 步骤：
 *  1. 按 HAL 状态映射成功、忙、超时或应答错误。
 *
 * @param[in]  handle               : HAL I2C 句柄。
 * @param[in]  status               : 待转换的 HAL 返回状态。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
static en_core_i2c_status_t i2c_bus_from_hal(
    I2C_HandleTypeDef *handle,
    HAL_StatusTypeDef  status)
{
    if (HAL_OK == status)
    {
        return CORE_I2C_OK;
    }
    if (HAL_BUSY == status)
    {
        return CORE_I2C_ERR_BUSY;
    }
    if (HAL_TIMEOUT == status)
    {
        return CORE_I2C_ERR_TIMEOUT;
    }
    if ((NULL != handle) &&
        (0U != (HAL_I2C_GetError(handle) & HAL_I2C_ERROR_AF)))
    {
        return CORE_I2C_ERR_NACK;
    }

    return CORE_I2C_ERROR;
}

/**
 * @brief 将 HAL I2C 错误码转换为统一状态。
 *
 * 步骤：
 *  1. 优先识别 NACK 和超时，其余归入通用错误。
 *
 * @param[in]  error_code           : HAL_I2C_GetError 返回的错误码。
 *
 * @return en_core_i2c_status_t : 对应的统一状态。
 */
static en_core_i2c_status_t i2c_bus_status_from_error(
    uint32_t error_code)
{
    if (0U != (error_code & HAL_I2C_ERROR_AF))
    {
        return CORE_I2C_ERR_NACK;
    }
    if (0U != (error_code & HAL_I2C_ERROR_TIMEOUT))
    {
        return CORE_I2C_ERR_TIMEOUT;
    }

    return CORE_I2C_ERROR;
}

/**
 * @brief 根据 HAL 句柄查找逻辑槽位
 *
 * 步骤：
 *  1. 遍历槽位配置表，比较硬件句柄
 *
 * @param[in]  p_handle             : HAL I2C 句柄
 * @param[out] p_port               : 匹配到的逻辑槽位索引，不得为空
 *
 * @return st_core_i2c_slot_t * : 匹配的槽位；未找到返回 NULL
 */
static st_core_i2c_slot_t *i2c_bus_slot_from_handle(
    I2C_HandleTypeDef *p_handle,
    core_i2c_port_id_t *p_port)
{
    uint32_t i;

    if ((NULL == p_handle) || (NULL == p_port))
    {
        return NULL;
    }

    for (i = 0U; i < CORE_I2C_PORT_MAX; i++)
    {
        if ((CORE_I2C_TYPE_HARDWARE == g_i2c_slots[i].type) &&
            (g_i2c_slots[i].config.hardware.handle == p_handle))
        {
            *p_port = (core_i2c_port_id_t)i;
            return &g_i2c_slots[i];
        }
    }

    return NULL;
}

/**
 * @brief 调用当前传输方向对应的完成回调。
 *
 * 步骤：
 *  1. 读方向调用 rx 回调，写方向调用 tx 回调。
 *  2. 回调为空时跳过。
 *
 * @param[in]  slot                 : 已验证配置的槽位，不得为空。
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  status               : 本次传输结果。
 *
 * @return void : 无返回值。
 */
static void i2c_bus_notify_complete(
    st_core_i2c_slot_t *slot,
    core_i2c_port_id_t  port,
    en_core_i2c_status_t status)
{
    if (CORE_I2C_DMA_KIND_READ == slot->dma.kind)
    {
        if (NULL != g_i2c_slots[port].pf_rx)
        {
            g_i2c_slots[port].pf_rx(
                port,
                slot->dma.device_address_7bit,
                status,
                slot->dma.p_buffer,
                slot->dma.size);
        }
    }
    else if (CORE_I2C_DMA_KIND_WRITE == slot->dma.kind)
    {
        if (NULL != g_i2c_slots[port].pf_tx)
        {
            g_i2c_slots[port].pf_tx(
                port,
                slot->dma.device_address_7bit,
                status);
        }
    }
    else
    {
        /* 不做任何事。 */
    }
}
#endif

//********************** Static Functions end ********************************//

//**************** Function Definitions begin ********************************//
//**************************** 公共接口 begin *******************************//

/**
 * @brief 初始化 I2C Bus 的软件状态。
 *
 * 步骤：
 *  1. 清空回调表。
 *  2. 清空槽位 busy 和 DMA 上下文。
 *
 * @return en_core_i2c_status_t : 固定返回 CORE_I2C_OK。
 */
en_core_i2c_status_t i2c_bus_init(void)
{
    uint32_t i;

    for (i = 0U; i < CORE_I2C_PORT_MAX; i++)
    {
        g_i2c_slots[i].pf_rx = NULL;
        g_i2c_slots[i].pf_tx = NULL;
        g_i2c_slots[i].pf_error = NULL;
        g_i2c_slots[i].busy = 0U;
#if (CORE_I2C_HW_ENABLE != 0)
        g_i2c_slots[i].dma.p_buffer = NULL;
        g_i2c_slots[i].dma.size = 0U;
        g_i2c_slots[i].dma.device_address_7bit = 0U;
        g_i2c_slots[i].dma.kind = CORE_I2C_DMA_KIND_NONE;
#endif
    }

    return CORE_I2C_OK;
}

/**
 * @brief 查询逻辑总线是否正在传输。
 *
 * 步骤：
 *  1. 检查槽位范围并读取 busy 标志。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return uint8_t : 1 表示忙，0 表示空闲或槽位非法。
 */
uint8_t i2c_bus_is_busy(core_i2c_port_id_t port)
{
    if (port >= CORE_I2C_PORT_MAX)
    {
        return 0U;
    }

    return (0U != g_i2c_slots[port].busy) ? 1U : 0U;
}

/**
 * @brief 查询逻辑总线是否为软件 I2C 槽位。
 *
 * 步骤：
 *  1. 检查槽位范围。
 *  2. 返回槽位类型是否为软件 I2C。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return uint8_t : 非零表示软件 I2C 槽位。
 */
uint8_t i2c_bus_is_software(core_i2c_port_id_t port)
{
    if (port >= CORE_I2C_PORT_MAX)
    {
        return 0U;
    }

    return (CORE_I2C_TYPE_SOFTWARE == g_i2c_slots[port].type) ? 1U : 0U;
}

#if (CORE_I2C_HW_ENABLE != 0)
/**
 * @brief 获取逻辑总线对应的 HAL I2C 句柄。
 *
 * 步骤：
 *  1. 检查槽位范围和总线类型。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return I2C_HandleTypeDef * : 硬件句柄；软件槽位或非法槽位返回 NULL。
 */
I2C_HandleTypeDef *i2c_bus_get_handle(core_i2c_port_id_t port)
{
    if (port >= CORE_I2C_PORT_MAX)
    {
        return NULL;
    }

    if (CORE_I2C_TYPE_HARDWARE != g_i2c_slots[port].type)
    {
        return NULL;
    }

    return g_i2c_slots[port].config.hardware.handle;
}
#endif

//***************************** 公共接口 end ********************************//

//**************************** 死等接口 begin *******************************//

/**
 * @brief 向设备写入连续数据。
 *
 * 步骤：
 *  1. 检查参数和 busy 状态。
 *  2. 分发硬件 HAL 或软件 I2C 事务。
 *  3. 传输结束后清 busy。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  data                 : 发送缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  timeout_ms           : HAL 操作的毫秒超时。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
en_core_i2c_status_t i2c_bus_write(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit,
    const uint8_t      *data,
    uint16_t            size,
    uint32_t            timeout_ms)
{
    (void)timeout_ms;
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t  status;

    //-------------------------参数检查 begin-------------------------//
    status = i2c_bus_validate_transfer(
        port, device_address_7bit, data, size);
    if (CORE_I2C_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];

#if (CORE_I2C_HW_ENABLE != 0)
    if (CORE_I2C_TYPE_HARDWARE == slot->type)
    {
        return i2c_bus_write_direct(slot->config.hardware.handle,
                                    device_address_7bit,
                                    data,
                                    size,
                                    timeout_ms);
    }
#endif

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    slot->busy = 1U;
    status = soft_i2c_write(&slot->config.software,
                            device_address_7bit,
                            data,
                            size);
    slot->busy = 0U;
    return status;
}

/**
 * @brief 从设备读取连续数据。
 *
 * 步骤：
 *  1. 检查参数和 busy 状态。
 *  2. 分发硬件 HAL 或软件 I2C 事务。
 *  3. 传输结束后清 busy。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[out] data                 : 接收缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  timeout_ms           : HAL 操作的毫秒超时。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
en_core_i2c_status_t i2c_bus_read(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit,
    uint8_t            *data,
    uint16_t            size,
    uint32_t            timeout_ms)
{
    (void)timeout_ms;
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t  status;

    //-------------------------参数检查 begin-------------------------//
    status = i2c_bus_validate_transfer(
        port, device_address_7bit, data, size);
    if (CORE_I2C_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];

#if (CORE_I2C_HW_ENABLE != 0)
    if (CORE_I2C_TYPE_HARDWARE == slot->type)
    {
        return i2c_bus_read_direct(slot->config.hardware.handle,
                                   device_address_7bit,
                                   data,
                                   size,
                                   timeout_ms);
    }
#endif

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    slot->busy = 1U;
    status = soft_i2c_read(&slot->config.software,
                           device_address_7bit,
                           data,
                           size);
    slot->busy = 0U;
    return status;
}

/**
 * @brief 向设备指定内部地址写入数据。
 *
 * 步骤：
 *  1. 检查参数和 busy 状态。
 *  2. 分发硬件 HAL 或软件 I2C 事务。
 *  3. 传输结束后清 busy。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[in]  data                 : 发送缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  timeout_ms           : HAL 操作的毫秒超时。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
en_core_i2c_status_t i2c_bus_mem_write(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *data,
    uint16_t                     size,
    uint32_t                     timeout_ms)
{
    (void)timeout_ms;
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t  status;

    //-------------------------参数检查 begin-------------------------//
    status = i2c_bus_validate_transfer(
        port, device_address_7bit, data, size);
    if (CORE_I2C_OK != status)
    {
        return status;
    }
    status = i2c_bus_validate_address_size(address_size);
    if (CORE_I2C_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];

#if (CORE_I2C_HW_ENABLE != 0)
    if (CORE_I2C_TYPE_HARDWARE == slot->type)
    {
        return i2c_bus_mem_write_direct(slot->config.hardware.handle,
                                        device_address_7bit,
                                        memory_address,
                                        address_size,
                                        data,
                                        size,
                                        timeout_ms);
    }
#endif

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    slot->busy = 1U;
    status = soft_i2c_mem_write(&slot->config.software,
                                device_address_7bit,
                                memory_address,
                                address_size,
                                data,
                                size);
    slot->busy = 0U;
    return status;
}

/**
 * @brief 从设备指定内部地址读取数据。
 *
 * 步骤：
 *  1. 检查参数和 busy 状态。
 *  2. 分发硬件 HAL 或软件 I2C 事务。
 *  3. 传输结束后清 busy。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[out] data                 : 接收缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  timeout_ms           : HAL 操作的毫秒超时。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
en_core_i2c_status_t i2c_bus_mem_read(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *data,
    uint16_t                     size,
    uint32_t                     timeout_ms)
{
    (void)timeout_ms;
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t  status;

    //-------------------------参数检查 begin-------------------------//
    status = i2c_bus_validate_transfer(
        port, device_address_7bit, data, size);
    if (CORE_I2C_OK != status)
    {
        return status;
    }
    status = i2c_bus_validate_address_size(address_size);
    if (CORE_I2C_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];

#if (CORE_I2C_HW_ENABLE != 0)
    if (CORE_I2C_TYPE_HARDWARE == slot->type)
    {
        return i2c_bus_mem_read_direct(slot->config.hardware.handle,
                                       device_address_7bit,
                                       memory_address,
                                       address_size,
                                       data,
                                       size,
                                       timeout_ms);
    }
#endif

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    slot->busy = 1U;
    status = soft_i2c_mem_read(&slot->config.software,
                               device_address_7bit,
                               memory_address,
                               address_size,
                               data,
                               size);
    slot->busy = 0U;
    return status;
}

/**
 * @brief 探测设备是否应答其地址。
 *
 * 步骤：
 *  1. 检查参数和 busy 状态。
 *  2. 分发硬件 HAL 或软件 I2C 事务。
 *  3. 传输结束后清 busy。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  trials               : 最大探测次数。
 * @param[in]  timeout_ms           : HAL 操作的毫秒超时。
 *
 * @return en_core_i2c_status_t : 成功为 CORE_I2C_OK，否则为具体错误。
 */
en_core_i2c_status_t i2c_bus_is_ready(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit,
    uint32_t            trials,
    uint32_t            timeout_ms)
{
    (void)timeout_ms;
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t  status;

    //-------------------------参数检查 begin-------------------------//
    status = i2c_bus_validate_port(port);
    if (CORE_I2C_OK != status)
    {
        return status;
    }
    if ((device_address_7bit > 0x7FU) || (0U == trials))
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];

#if (CORE_I2C_HW_ENABLE != 0)
    if (CORE_I2C_TYPE_HARDWARE == slot->type)
    {
        return i2c_bus_is_ready_direct(slot->config.hardware.handle,
                                       device_address_7bit,
                                       trials,
                                       timeout_ms);
    }
#endif

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    slot->busy = 1U;
    status = soft_i2c_is_ready(&slot->config.software,
                               device_address_7bit,
                               trials);
    slot->busy = 0U;
    return status;
}

//***************************** 死等接口 end ********************************//

//************************** DMA 接口 begin *********************************//

/**
 * @brief 异步启动一次指定内部地址的 DMA 读。
 *
 * 步骤：
 *  1. 检查参数、槽位类型和 busy 状态。
 *  2. 占用 busy 并调用内部启动函数。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[out] p_data               : 接收缓冲区。
 * @param[in]  size                 : 传输字节数。
 *
 * @return en_core_i2c_status_t : CORE_I2C_OK 只代表启动成功。
 */
en_core_i2c_status_t i2c_bus_mem_read_dma(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size)
{
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t  status;

    //-------------------------参数检查 begin-------------------------//
    status = i2c_bus_validate_transfer(
        port, device_address_7bit, p_data, size);
    if (CORE_I2C_OK != status)
    {
        return status;
    }
    status = i2c_bus_validate_address_size(address_size);
    if (CORE_I2C_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];
    if (CORE_I2C_TYPE_HARDWARE != slot->type)
    {
        return CORE_I2C_ERR_UNSUPPORTED;
    }

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    return i2c_bus_mem_read_dma_direct(slot->config.hardware.handle,
                                       device_address_7bit,
                                       memory_address,
                                       address_size,
                                       p_data,
                                       size);
#else
    (void)port;
    (void)device_address_7bit;
    (void)memory_address;
    (void)address_size;
    (void)p_data;
    (void)size;
    return CORE_I2C_ERR_UNSUPPORTED;
#endif
}

/**
 * @brief 异步启动一次指定内部地址的 DMA 写。
 *
 * 步骤：
 *  1. 检查参数、槽位类型和 busy 状态。
 *  2. 占用 busy 并调用内部启动函数。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[in]  p_data               : 发送缓冲区。
 * @param[in]  size                 : 传输字节数。
 *
 * @return en_core_i2c_status_t : CORE_I2C_OK 只代表启动成功。
 */
en_core_i2c_status_t i2c_bus_mem_write_dma(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *p_data,
    uint16_t                     size)
{
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_slot_t   *slot;
    en_core_i2c_status_t  status;

    //-------------------------参数检查 begin-------------------------//
    status = i2c_bus_validate_transfer(
        port, device_address_7bit, p_data, size);
    if (CORE_I2C_OK != status)
    {
        return status;
    }
    status = i2c_bus_validate_address_size(address_size);
    if (CORE_I2C_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];
    if (CORE_I2C_TYPE_HARDWARE != slot->type)
    {
        return CORE_I2C_ERR_UNSUPPORTED;
    }

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    return i2c_bus_mem_write_dma_direct(slot->config.hardware.handle,
                                        device_address_7bit,
                                        memory_address,
                                        address_size,
                                        p_data,
                                        size);
#else
    (void)port;
    (void)device_address_7bit;
    (void)memory_address;
    (void)address_size;
    (void)p_data;
    (void)size;
    return CORE_I2C_ERR_UNSUPPORTED;
#endif
}



/**
 * @brief 中止当前 DMA 传输。
 *
 * 步骤：
 *  1. 检查槽位、总线类型和 busy 状态。
 *  2. 调用 HAL_I2C_Master_Abort_IT 请求停止并发送 STOP。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return en_core_i2c_status_t : 中止请求结果。
 */
en_core_i2c_status_t i2c_bus_abort_dma(core_i2c_port_id_t port)
{
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_slot_t *slot;

    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    slot = &g_i2c_slots[port];
    if (CORE_I2C_TYPE_HARDWARE != slot->type)
    {
        return CORE_I2C_ERR_UNSUPPORTED;
    }

    return i2c_bus_abort_dma_direct(slot->config.hardware.handle);
#else
    (void)port;
    return CORE_I2C_ERR_UNSUPPORTED;
#endif
}

/**
 * @brief 强制复位总线状态，用于 abort 完成中断丢失后的兜底恢复。
 *
 * 步骤：
 *  1. 检查槽位范围和板级资源。
 *  2. 硬件槽位重新 Init HAL，清 DMA 上下文。
 *  3. 清 busy，允许后续传输重新开始。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return en_core_i2c_status_t : 复位结果。
 */
en_core_i2c_status_t i2c_bus_recover(core_i2c_port_id_t port)
{
    st_core_i2c_slot_t *slot;

    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    slot = &g_i2c_slots[port];
    if (CORE_I2C_TYPE_NONE == slot->type)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }
    //--------------------------参数检查 end--------------------------//

#if (CORE_I2C_HW_ENABLE != 0)
    if (CORE_I2C_TYPE_HARDWARE == slot->type)
    {
        if (NULL == slot->config.hardware.handle)
        {
            return CORE_I2C_ERR_NOT_CONFIGURED;
        }

        (void)HAL_I2C_DeInit(slot->config.hardware.handle);
        if (HAL_OK != HAL_I2C_Init(slot->config.hardware.handle))
        {
            return CORE_I2C_ERROR;
        }

        slot->dma.p_buffer = NULL;
        slot->dma.size = 0U;
        slot->dma.device_address_7bit = 0U;
        slot->dma.kind = CORE_I2C_DMA_KIND_NONE;
    }
#endif

    slot->busy = 0U;
    return CORE_I2C_OK;
}

//*************************** DMA 接口 end **********************************//

//*************************** 回调注册 begin *********************************//

/**
 * @brief 注册读完成回调。
 *
 * 步骤：
 *  1. 检查槽位范围。
 *  2. 保存回调函数指针。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  pf_callback          : 读完成回调，NULL 表示取消注册。
 *
 * @return en_core_i2c_status_t : 注册结果。
 */
en_core_i2c_status_t i2c_bus_register_rx_callback(
    core_i2c_port_id_t port,
    i2c_rx_callback_t  pf_callback)
{
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    g_i2c_slots[port].pf_rx = pf_callback;
    return CORE_I2C_OK;
}

/**
 * @brief 注册写完成回调。
 *
 * 步骤：
 *  1. 检查槽位范围。
 *  2. 保存回调函数指针。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  pf_callback          : 写完成回调，NULL 表示取消注册。
 *
 * @return en_core_i2c_status_t : 注册结果。
 */
en_core_i2c_status_t i2c_bus_register_tx_callback(
    core_i2c_port_id_t port,
    i2c_tx_callback_t  pf_callback)
{
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    g_i2c_slots[port].pf_tx = pf_callback;
    return CORE_I2C_OK;
}

/**
 * @brief 注册错误回调。
 *
 * 步骤：
 *  1. 检查槽位范围。
 *  2. 保存回调函数指针。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  pf_callback          : 错误回调，NULL 表示取消注册。
 *
 * @return en_core_i2c_status_t : 注册结果。
 */
en_core_i2c_status_t i2c_bus_register_error_callback(
    core_i2c_port_id_t    port,
    i2c_error_callback_t  pf_callback)
{
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    g_i2c_slots[port].pf_error = pf_callback;
    return CORE_I2C_OK;
}

#if (CORE_I2C_HW_ENABLE != 0)
//**************************** 回调注册 end **********************************//

//*************************** 中断转发 begin *********************************//

/**
 * @brief 转发 HAL 内存读完成事件。
 *
 * 步骤：
 *  1. 按句柄查找槽位。
 *  2. 清 busy 并调用读完成回调。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 *
 * @return en_core_i2c_status_t : 分发结果。
 */
en_core_i2c_status_t i2c_bus_irq_mem_rx_complete(
    I2C_HandleTypeDef *p_handle)
{
    st_core_i2c_slot_t *slot;
    core_i2c_port_id_t  port;

    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (0U == slot->busy)
    {
        return CORE_I2C_OK;
    }

    slot->busy = 0U;
    i2c_bus_notify_complete(slot, port, CORE_I2C_OK);
    return CORE_I2C_OK;
}

/**
 * @brief 转发 HAL 内存写完成事件。
 *
 * 步骤：
 *  1. 按句柄查找槽位。
 *  2. 清 busy 并调用写完成回调。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 *
 * @return en_core_i2c_status_t : 分发结果。
 */
en_core_i2c_status_t i2c_bus_irq_mem_tx_complete(
    I2C_HandleTypeDef *p_handle)
{
    st_core_i2c_slot_t *slot;
    core_i2c_port_id_t  port;

    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (0U == slot->busy)
    {
        return CORE_I2C_OK;
    }

    slot->busy = 0U;
    i2c_bus_notify_complete(slot, port, CORE_I2C_OK);
    return CORE_I2C_OK;
}

/**
 * @brief 转发 HAL I2C 错误事件。
 *
 * 步骤：
 *  1. 按句柄查找槽位并映射错误码。
 *  2. 清 busy，调用完成回调和错误回调。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 *
 * @return en_core_i2c_status_t : 分发结果。
 */
en_core_i2c_status_t i2c_bus_irq_error(
    I2C_HandleTypeDef *p_handle)
{
    st_core_i2c_slot_t   *slot;
    core_i2c_port_id_t    port;
    en_core_i2c_status_t  status;
    uint32_t              error_code;
    uint8_t               device_address_7bit;

    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    error_code = HAL_I2C_GetError(p_handle);
    status = i2c_bus_status_from_error(error_code);
    device_address_7bit = slot->dma.device_address_7bit;

    slot->busy = 0U;
    i2c_bus_notify_complete(slot, port, status);

    if (NULL != g_i2c_slots[port].pf_error)
    {
        g_i2c_slots[port].pf_error(
            port, device_address_7bit, error_code);
    }

    return CORE_I2C_OK;
}

/**
 * @brief 转发 HAL I2C 中止完成事件。
 *
 * 步骤：
 *  1. 按句柄查找槽位。
 *  2. 清 busy 并通知上层 TIMEOUT。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 *
 * @return en_core_i2c_status_t : 分发结果。
 */
en_core_i2c_status_t i2c_bus_irq_abort_complete(
    I2C_HandleTypeDef *p_handle)
{
    st_core_i2c_slot_t *slot;
    core_i2c_port_id_t  port;

    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    slot->busy = 0U;
    i2c_bus_notify_complete(slot, port, CORE_I2C_ERR_TIMEOUT);
    return CORE_I2C_OK;
}
#else
/**
 * @brief 未启用硬件 I2C 时的空实现。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 *
 * @return en_core_i2c_status_t : 固定返回 CORE_I2C_ERR_UNSUPPORTED。
 */
en_core_i2c_status_t i2c_bus_irq_mem_rx_complete(
    I2C_HandleTypeDef *p_handle)
{
    (void)p_handle;
    return CORE_I2C_ERR_UNSUPPORTED;
}

/**
 * @brief 未启用硬件 I2C 时的空实现。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 *
 * @return en_core_i2c_status_t : 固定返回 CORE_I2C_ERR_UNSUPPORTED。
 */
en_core_i2c_status_t i2c_bus_irq_mem_tx_complete(
    I2C_HandleTypeDef *p_handle)
{
    (void)p_handle;
    return CORE_I2C_ERR_UNSUPPORTED;
}

/**
 * @brief 未启用硬件 I2C 时的空实现。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 *
 * @return en_core_i2c_status_t : 固定返回 CORE_I2C_ERR_UNSUPPORTED。
 */
en_core_i2c_status_t i2c_bus_irq_error(
    I2C_HandleTypeDef *p_handle)
{
    (void)p_handle;
    return CORE_I2C_ERR_UNSUPPORTED;
}

/**
 * @brief 未启用硬件 I2C 时的空实现。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 *
 * @return en_core_i2c_status_t : 固定返回 CORE_I2C_ERR_UNSUPPORTED。
 */
en_core_i2c_status_t i2c_bus_irq_abort_complete(
    I2C_HandleTypeDef *p_handle)
{
    (void)p_handle;
    return CORE_I2C_ERR_UNSUPPORTED;
}
#endif

//*************************** 中断转发 end ***********************************//

//************************** 内部 HAL 层 begin *******************************//

/**
 * @brief 直接用 HAL 句柄启动一次 DMA 读，不做逻辑槽位和参数校验。
 *
 * 步骤：
 *  1. 由 HAL 句柄反查槽位。
 *  2. 检查 HAL 状态和 BUSY 标志。
 *  3. 保存 DMA 上下文，占用 busy 并启动 DMA。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[out] p_data               : 接收缓冲区。
 * @param[in]  size                 : 传输字节数。
 *
 * @return en_core_i2c_status_t : 启动结果。
 */
static en_core_i2c_status_t i2c_bus_mem_read_dma_direct(
    I2C_HandleTypeDef *p_handle,
    uint8_t device_address_7bit,
    uint16_t memory_address,
    en_core_i2c_mem_addr_size_t address_size,
    uint8_t *p_data,
    uint16_t size)
{
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_slot_t *slot;
    core_i2c_port_id_t  port;
    HAL_StatusTypeDef   hal_status;
    uint16_t            hal_address_size;

    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (HAL_I2C_STATE_READY != HAL_I2C_GetState(p_handle))
    {
        return CORE_I2C_ERR_BUSY;
    }

    if (RESET != __HAL_I2C_GET_FLAG(p_handle, I2C_FLAG_BUSY))
    {
        return CORE_I2C_ERR_BUSY;
    }

    slot->dma.p_buffer = p_data;
    slot->dma.size = size;
    slot->dma.device_address_7bit = device_address_7bit;
    slot->dma.kind = CORE_I2C_DMA_KIND_READ;

    hal_address_size = (CORE_I2C_MEM_ADDR_8BIT == address_size)
                           ? I2C_MEMADD_SIZE_8BIT
                           : I2C_MEMADD_SIZE_16BIT;

    slot->busy = 1U;
    hal_status = HAL_I2C_Mem_Read_DMA(p_handle,
                                      (uint16_t)device_address_7bit << 1U,
                                      memory_address,
                                      hal_address_size,
                                      p_data,
                                      size);
    if (HAL_OK != hal_status)
    {
        slot->busy = 0U;
        return i2c_bus_from_hal(p_handle, hal_status);
    }

    return CORE_I2C_OK;
#else
    (void)p_handle;
    (void)device_address_7bit;
    (void)memory_address;
    (void)address_size;
    (void)p_data;
    (void)size;
    return CORE_I2C_ERR_UNSUPPORTED;
#endif
}

/**
 * @brief 直接用 HAL 句柄启动一次 DMA 写，不做逻辑槽位和参数校验。
 *
 * 步骤：
 *  1. 由 HAL 句柄反查槽位。
 *  2. 检查 HAL 状态和 BUSY 标志。
 *  3. 保存 DMA 上下文，占用 busy 并启动 DMA。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[in]  p_data               : 发送缓冲区。
 * @param[in]  size                 : 传输字节数。
 *
 * @return en_core_i2c_status_t : 启动结果。
 */
static en_core_i2c_status_t i2c_bus_mem_write_dma_direct(
    I2C_HandleTypeDef *p_handle,
    uint8_t device_address_7bit,
    uint16_t memory_address,
    en_core_i2c_mem_addr_size_t address_size,
    const uint8_t *p_data,
    uint16_t size)
{
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_slot_t *slot;
    core_i2c_port_id_t  port;
    HAL_StatusTypeDef   hal_status;
    uint16_t            hal_address_size;

    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (HAL_I2C_STATE_READY != HAL_I2C_GetState(p_handle))
    {
        return CORE_I2C_ERR_BUSY;
    }

    if (RESET != __HAL_I2C_GET_FLAG(p_handle, I2C_FLAG_BUSY))
    {
        return CORE_I2C_ERR_BUSY;
    }

    slot->dma.p_buffer = (uint8_t *)p_data;
    slot->dma.size = size;
    slot->dma.device_address_7bit = device_address_7bit;
    slot->dma.kind = CORE_I2C_DMA_KIND_WRITE;

    hal_address_size = (CORE_I2C_MEM_ADDR_8BIT == address_size)
                           ? I2C_MEMADD_SIZE_8BIT
                           : I2C_MEMADD_SIZE_16BIT;

    slot->busy = 1U;
    hal_status = HAL_I2C_Mem_Write_DMA(p_handle,
                                       (uint16_t)device_address_7bit << 1U,
                                       memory_address,
                                       hal_address_size,
                                       (uint8_t *)p_data,
                                       size);
    if (HAL_OK != hal_status)
    {
        slot->busy = 0U;
        return i2c_bus_from_hal(p_handle, hal_status);
    }

    return CORE_I2C_OK;
#else
    (void)p_handle;
    (void)device_address_7bit;
    (void)memory_address;
    (void)address_size;
    (void)p_data;
    (void)size;
    return CORE_I2C_ERR_UNSUPPORTED;
#endif
}
/**
 * @brief 直接用 HAL 句柄发送一段数据，跳过逻辑槽位和参数校验。
 *
 * 步骤：
 *  1. 由 HAL 句柄反查槽位。
 *  2. 占用 busy 并调用 HAL 阻塞发送。
 *  3. 释放 busy 并返回映射后的状态。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  data                 : 发送缓冲区。
 * @param[in]  size                 : 发送字节数。
 * @param[in]  timeout_ms           : HAL 阻塞超时。
 *
 * @return en_core_i2c_status_t : 传输结果。
 */
static en_core_i2c_status_t i2c_bus_write_direct(
    I2C_HandleTypeDef *p_handle,
    uint8_t device_address_7bit,
    const uint8_t *data,
    uint16_t size,
    uint32_t timeout_ms)
{
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_slot_t   *slot;
    core_i2c_port_id_t    port;
    en_core_i2c_status_t  status;

    //找对应的IIC结构体获取其他信息
    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    slot->busy = 1U;
    status = i2c_bus_from_hal(
        p_handle,
        HAL_I2C_Master_Transmit(p_handle,
                                (uint16_t)device_address_7bit << 1U,
                                (uint8_t *)data,
                                size,
                                timeout_ms));
    slot->busy = 0U;

    return status;
#else
    (void)p_handle;
    (void)device_address_7bit;
    (void)data;
    (void)size;
    (void)timeout_ms;
    return CORE_I2C_ERR_UNSUPPORTED;
#endif
}

/**
 * @brief 直接用 HAL 句柄接收一段数据，跳过逻辑槽位和参数校验。
 *
 * 步骤：
 *  1. 由 HAL 句柄反查槽位。
 *  2. 占用 busy 并调用 HAL 阻塞接收。
 *  3. 释放 busy 并返回映射后的状态。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[out] data                 : 接收缓冲区。
 * @param[in]  size                 : 接收字节数。
 * @param[in]  timeout_ms           : HAL 阻塞超时。
 *
 * @return en_core_i2c_status_t : 传输结果。
 */
static en_core_i2c_status_t i2c_bus_read_direct(
    I2C_HandleTypeDef *p_handle,
    uint8_t device_address_7bit,
    uint8_t *data,
    uint16_t size,
    uint32_t timeout_ms)
{
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_slot_t   *slot;
    core_i2c_port_id_t    port;
    en_core_i2c_status_t  status;

    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    slot->busy = 1U;
    status = i2c_bus_from_hal(
        p_handle,
        HAL_I2C_Master_Receive(p_handle,
                               (uint16_t)device_address_7bit << 1U,
                               data,
                               size,
                               timeout_ms));
    slot->busy = 0U;

    return status;
#else
    (void)p_handle;
    (void)device_address_7bit;
    (void)data;
    (void)size;
    (void)timeout_ms;
    return CORE_I2C_ERR_UNSUPPORTED;
#endif
}

/**
 * @brief 直接用 HAL 句柄写寄存器，跳过逻辑槽位和参数校验。
 *
 * 步骤：
 *  1. 由 HAL 句柄反查槽位。
 *  2. 占用 busy 并调用 HAL 阻塞写寄存器。
 *  3. 释放 busy 并返回映射后的状态。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[in]  data                 : 发送缓冲区。
 * @param[in]  size                 : 发送字节数。
 * @param[in]  timeout_ms           : HAL 阻塞超时。
 *
 * @return en_core_i2c_status_t : 传输结果。
 */
static en_core_i2c_status_t i2c_bus_mem_write_direct(
    I2C_HandleTypeDef *p_handle,
    uint8_t device_address_7bit,
    uint16_t memory_address,
    en_core_i2c_mem_addr_size_t address_size,
    const uint8_t *data,
    uint16_t size,
    uint32_t timeout_ms)
{
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_slot_t   *slot;
    core_i2c_port_id_t    port;
    en_core_i2c_status_t  status;
    uint16_t              hal_address_size;

    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    hal_address_size = (CORE_I2C_MEM_ADDR_8BIT == address_size)
                           ? I2C_MEMADD_SIZE_8BIT
                           : I2C_MEMADD_SIZE_16BIT;

    slot->busy = 1U;
    status = i2c_bus_from_hal(
        p_handle,
        HAL_I2C_Mem_Write(p_handle,
                          (uint16_t)device_address_7bit << 1U,
                          memory_address,
                          hal_address_size,
                          (uint8_t *)data,
                          size,
                          timeout_ms));
    slot->busy = 0U;

    return status;
#else
    (void)p_handle;
    (void)device_address_7bit;
    (void)memory_address;
    (void)address_size;
    (void)data;
    (void)size;
    (void)timeout_ms;
    return CORE_I2C_ERR_UNSUPPORTED;
#endif
}

/**
 * @brief 直接用 HAL 句柄读寄存器，跳过逻辑槽位和参数校验。
 *
 * 步骤：
 *  1. 由 HAL 句柄反查槽位。
 *  2. 占用 busy 并调用 HAL 阻塞读寄存器。
 *  3. 释放 busy 并返回映射后的状态。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[out] data                 : 接收缓冲区。
 * @param[in]  size                 : 接收字节数。
 * @param[in]  timeout_ms           : HAL 阻塞超时。
 *
 * @return en_core_i2c_status_t : 传输结果。
 */
static en_core_i2c_status_t i2c_bus_mem_read_direct(
    I2C_HandleTypeDef *p_handle,
    uint8_t device_address_7bit,
    uint16_t memory_address,
    en_core_i2c_mem_addr_size_t address_size,
    uint8_t *data,
    uint16_t size,
    uint32_t timeout_ms)
{
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_slot_t   *slot;
    core_i2c_port_id_t    port;
    en_core_i2c_status_t  status;
    uint16_t              hal_address_size;

    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    hal_address_size = (CORE_I2C_MEM_ADDR_8BIT == address_size)
                           ? I2C_MEMADD_SIZE_8BIT
                           : I2C_MEMADD_SIZE_16BIT;

    slot->busy = 1U;
    status = i2c_bus_from_hal(
        p_handle,
        HAL_I2C_Mem_Read(p_handle,
                         (uint16_t)device_address_7bit << 1U,
                         memory_address,
                         hal_address_size,
                         data,
                         size,
                         timeout_ms));
    slot->busy = 0U;

    return status;
#else
    (void)p_handle;
    (void)device_address_7bit;
    (void)memory_address;
    (void)address_size;
    (void)data;
    (void)size;
    (void)timeout_ms;
    return CORE_I2C_ERR_UNSUPPORTED;
#endif
}

/**
 * @brief 直接用 HAL 句柄探测设备应答，跳过逻辑槽位和参数校验。
 *
 * 步骤：
 *  1. 由 HAL 句柄反查槽位。
 *  2. 占用 busy 并调用 HAL 探测设备地址。
 *  3. 释放 busy 并返回映射后的状态。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  trials               : 最大探测次数。
 * @param[in]  timeout_ms           : HAL 阻塞超时。
 *
 * @return en_core_i2c_status_t : 探测结果。
 */
static en_core_i2c_status_t i2c_bus_is_ready_direct(
    I2C_HandleTypeDef *p_handle,
    uint8_t device_address_7bit,
    uint32_t trials,
    uint32_t timeout_ms)
{
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_slot_t   *slot;
    core_i2c_port_id_t    port;
    en_core_i2c_status_t  status;

    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (0U != slot->busy)
    {
        return CORE_I2C_ERR_BUSY;
    }

    slot->busy = 1U;
    status = i2c_bus_from_hal(
        p_handle,
        HAL_I2C_IsDeviceReady(p_handle,
                              (uint16_t)device_address_7bit << 1U,
                              trials,
                              timeout_ms));
    slot->busy = 0U;

    return status;
#else
    (void)p_handle;
    (void)device_address_7bit;
    (void)trials;
    (void)timeout_ms;
    return CORE_I2C_ERR_UNSUPPORTED;
#endif
}

/**
 * @brief 直接用 HAL 句柄中止 DMA 传输，跳过逻辑槽位和 busy 检查。
 *
 * 步骤：
 *  1. 由 HAL 句柄反查槽位。
 *  2. 用槽位里保存的设备地址发起中止。
 *
 * @param[in]  p_handle             : HAL I2C 句柄。
 *
 * @return en_core_i2c_status_t : 中止请求结果。
 */
static en_core_i2c_status_t i2c_bus_abort_dma_direct(
    I2C_HandleTypeDef *p_handle)
{
#if (CORE_I2C_HW_ENABLE != 0)
    st_core_i2c_slot_t *slot;
    core_i2c_port_id_t  port;

    slot = i2c_bus_slot_from_handle(p_handle, &port);
    if (NULL == slot)
    {
        return CORE_I2C_ERR_NOT_CONFIGURED;
    }

    if (0U == slot->busy)
    {
        return CORE_I2C_OK;
    }

    if (HAL_OK != HAL_I2C_Master_Abort_IT(
                     p_handle,
                     (uint16_t)slot->dma.device_address_7bit << 1U))
    {
        return CORE_I2C_ERROR;
    }

    return CORE_I2C_OK;
#else
    (void)p_handle;
    return CORE_I2C_ERR_UNSUPPORTED;
#endif
}
//*************************** 内部 HAL 层 end ********************************//

//****************** Function Definitions end ********************************//

//******************************* End of File ********************************//
