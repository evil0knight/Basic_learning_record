/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_encoder_driver.c
 *
 * @par dependencies
 * - bsp_encoder_driver.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 实现 AS5600 的同步 I2C 访问和选定功能。
 *
 * Processing flow:
 *
 * 检查接口 -> 读写寄存器 -> 等待配置生效 -> 返回状态。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "bsp_encoder_driver.h"
#include "../../../04_platform/platform_mcu/critical/inc/critical_port.h"
#include <stddef.h>
//******************************* Includes end *******************************//

//************************** Static Functions begin **************************//
static void bsp_encoder_dma_done(
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    en_core_i2c_status_t  status,
    void                 *p_user_data);

static bsp_encoder_status_t bsp_encoder_start_dma_with_buffer(
    bsp_encoder_driver_t *p_driver,
    uint8_t              *p_buffer);

static bsp_encoder_status_t bsp_encoder_start_dma_once(
    bsp_encoder_driver_t *p_driver);

/**
 * @brief 检查 Driver 和必须挂载的接口。
 * @param[in] p_driver : Driver 对象。
 * @return bsp_encoder_status_t : 参数错误、未挂载或成功。
 */
static bsp_encoder_status_t bsp_encoder_check(
    const bsp_encoder_driver_t *p_driver)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_driver)
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    if ((NULL == p_driver->iic_interface.pf_mem_read) ||
        (NULL == p_driver->iic_interface.pf_mem_write) ||
        (CORE_I2C_PORT_MAX <= p_driver->iic_interface.port) ||
        (NULL == p_driver->pf_delay_ms))
    {
        return BSP_ENCODER_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//
    return BSP_ENCODER_OK;
}

/**
 * @brief 将平台 I2C 状态转换为编码器状态，避免枚举数值混用。
 * @param[in] status : 平台传输结果。
 * @return bsp_encoder_status_t : 编码器状态。
 */
static bsp_encoder_status_t bsp_encoder_iic_status(
    en_core_i2c_status_t status)
{
    switch (status)
    {
        case CORE_I2C_OK:
            return BSP_ENCODER_OK;
        case CORE_I2C_ERR_ARGUMENT:
            return BSP_ENCODER_ERR_ARGUMENT;
        case CORE_I2C_ERR_TIMEOUT:
            return BSP_ENCODER_ERR_TIMEOUT;
        case CORE_I2C_ERR_BUSY:
            return BSP_ENCODER_ERR_BUSY;
        case CORE_I2C_ERR_NOT_CONFIGURED:
            return BSP_ENCODER_ERR_STATE;
        case CORE_I2C_ERR_NACK:
            return BSP_ENCODER_ERR_NACK;
        default:
            return BSP_ENCODER_ERROR;
    }
}

/**
 * @brief 通过配置的端口同步读取 8 位地址寄存器。
 * @param[in] p_driver : Driver 对象。
 * @param[in] u8_register : 起始寄存器地址。
 * @param[out] p_data : 接收缓冲区。
 * @param[in] u16_size : 读取字节数。
 * @return bsp_encoder_status_t : 参数、接口或传输结果。
 */
static bsp_encoder_status_t bsp_encoder_iic_read(
    const bsp_encoder_driver_t *p_driver, uint8_t u8_register,
    uint8_t *p_data, uint16_t u16_size)
{
    bsp_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_data) || (0U == u16_size))
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    status = bsp_encoder_check(p_driver);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    return bsp_encoder_iic_status(p_driver->iic_interface.pf_mem_read(
        p_driver->iic_interface.port, BSP_ENCODER_IIC_ADDRESS,
        u8_register, CORE_I2C_MEM_ADDR_8BIT, p_data, u16_size,
        p_driver->iic_interface.u32_timeout_ms));
}

/**
 * @brief 通过配置的端口同步写入 8 位地址寄存器。
 * @param[in] p_driver : Driver 对象。
 * @param[in] u8_register : 起始寄存器地址。
 * @param[in] p_data : 发送缓冲区。
 * @param[in] u16_size : 写入字节数。
 * @return bsp_encoder_status_t : 参数、接口或传输结果。
 */
static bsp_encoder_status_t bsp_encoder_iic_write(
    const bsp_encoder_driver_t *p_driver, uint8_t u8_register,
    const uint8_t *p_data, uint16_t u16_size)
{
    bsp_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_data) || (0U == u16_size))
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    status = bsp_encoder_check(p_driver);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    return bsp_encoder_iic_status(p_driver->iic_interface.pf_mem_write(
        p_driver->iic_interface.port, BSP_ENCODER_IIC_ADDRESS,
        u8_register, CORE_I2C_MEM_ADDR_8BIT, p_data, u16_size,
        p_driver->iic_interface.u32_timeout_ms));
}

/**
 * @brief 在一次同步事务中读取高、低字节，失败时不更新输出。
 * @param[in] p_driver : Driver 对象。
 * @param[in] u8_register : 高字节寄存器地址。
 * @param[out] p_u16_value : 16 位寄存器值。
 * @return bsp_encoder_status_t : 参数、接口或传输结果。
 */
static bsp_encoder_status_t bsp_encoder_read_word(
    const bsp_encoder_driver_t *p_driver,
    uint8_t u8_register, uint16_t *p_u16_value)
{
    uint8_t a_u8_data[2] = {0U, 0U};
    bsp_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_u16_value)
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    status = bsp_encoder_check(p_driver);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_encoder_iic_read(p_driver, u8_register, a_u8_data, 2U);
    if (BSP_ENCODER_OK == status)
    {
        *p_u16_value = (uint16_t)(((uint16_t)a_u8_data[0] << 8U) |
                                (uint16_t)a_u8_data[1]);
    }
    return status;
}

/**
 * @brief 按掩码修改双字节寄存器，保留其余位并等待至少 1ms。
 * @param[in] p_driver : Driver 对象。
 * @param[in] u8_register : 高字节寄存器地址。
 * @param[in] u16_mask : 可修改位。
 * @param[in] u16_value : 要写入的位值。
 * @return bsp_encoder_status_t : 参数、传输或延时结果。
 */
static bsp_encoder_status_t bsp_encoder_update_word(
    const bsp_encoder_driver_t *p_driver, uint8_t u8_register,
    uint16_t u16_mask, uint16_t u16_value)
{
    uint16_t u16_current = 0U;
    uint8_t a_u8_data[2];
    bsp_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = bsp_encoder_check(p_driver);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_encoder_read_word(p_driver, u8_register, &u16_current);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    u16_current = (uint16_t)((u16_current & (uint16_t)(~u16_mask)) |
                            (u16_value & u16_mask));
    a_u8_data[0] = (uint8_t)(u16_current >> 8U);
    a_u8_data[1] = (uint8_t)u16_current;
    status = bsp_encoder_iic_write(p_driver, u8_register, a_u8_data, 2U);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    return p_driver->pf_delay_ms(1U);
}

/**
 * @brief 用指定写块启动一次角度 DMA 接收。
 *
 * @param[in,out] p_driver : Driver 对象。
 * @param[in] p_buffer      : AB 双缓冲写块地址。
 *
 * @return bsp_encoder_status_t : 启动结果。
 */
static bsp_encoder_status_t bsp_encoder_start_dma_with_buffer(
    bsp_encoder_driver_t *p_driver,
    uint8_t              *p_buffer)
{
    core_i2c_request_t *p_request;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_buffer) ||
        (NULL == p_driver->iic_interface.pf_submit))
    {
        return BSP_ENCODER_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    p_request = &p_driver->s_dma_req;
    p_request->port = p_driver->iic_interface.port;
    p_request->device_address_7bit = BSP_ENCODER_IIC_ADDRESS;
    p_request->memory_address = ENCODER_REG_ANGLE;
    p_request->address_size = CORE_I2C_MEM_ADDR_8BIT;
    p_request->p_data = p_buffer;
    p_request->size = 2U;
    p_request->is_write = 0U;
    p_request->timeout_ms = p_driver->iic_interface.u32_timeout_ms;
    p_request->pf_done = bsp_encoder_dma_done;
    p_request->p_user_data = p_driver;

    return bsp_encoder_iic_status(
        p_driver->iic_interface.pf_submit(p_request));
}

/**
 * @brief 从 AB 双缓冲取空闲写块并启动一笔 DMA。
 *
 * @param[in,out] p_driver : Driver 对象。
 *
 * @return bsp_encoder_status_t : 启动结果；上一笔在飞时返回 BUSY。
 * @note 本函数只在外部采样节拍调用，不在完成回调里自动续传。
 */
static bsp_encoder_status_t bsp_encoder_start_dma_once(
    bsp_encoder_driver_t *p_driver)
{
    ab_buffer_status_t  ab_status;
    bsp_encoder_status_t status;
    uint8_t             *p_buffer = NULL;
    size_t               capacity = 0U;
    uint32_t             irq_state;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_driver)
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    if (0U == p_driver->u8_dma_enable)
    {
        return BSP_ENCODER_OK;
    }
    //--------------------------参数检查 end--------------------------//

    (void)platform_critical_enter(&irq_state);
    if (0U != p_driver->u8_dma_busy)
    {
        (void)platform_critical_exit(irq_state);
        return BSP_ENCODER_ERR_BUSY;
    }
    p_driver->u8_dma_busy = 1U;
    (void)platform_critical_exit(irq_state);

    (void)platform_critical_enter(&irq_state);
    ab_status = ab_buffer_acquire_write(&p_driver->s_angle_ab,
                                        &p_buffer, &capacity);
    (void)platform_critical_exit(irq_state);

    if (AB_BUFFER_BUSY == ab_status)
    {
        p_driver->u8_dma_busy = 0U;
        return BSP_ENCODER_ERR_BUSY;
    }
    if ((AB_BUFFER_OK != ab_status) || (capacity < 2U))
    {
        p_driver->u8_dma_busy = 0U;
        return BSP_ENCODER_ERR_STATE;
    }

    status = bsp_encoder_start_dma_with_buffer(p_driver, p_buffer);
    if (BSP_ENCODER_OK != status)
    {
        (void)platform_critical_enter(&irq_state);
        (void)ab_buffer_abort_write(&p_driver->s_angle_ab);
        p_driver->u8_dma_busy = 0U;
        (void)platform_critical_exit(irq_state);
    }

    return status;
}

/**
 * @brief 角度 DMA 完成回调，只提交当前块并清除在飞标志。
 *
 * @param[in] port               : 逻辑总线槽位。
 * @param[in] device_address_7bit : 七位设备地址。
 * @param[in] status             : 本次传输结果。
 * @param[in] p_user_data        : Driver 对象。
 *
 * @return void : 无返回值。
 * @note 下一次采样由外部 5kHz 节拍重新启动，不在回调里续传。
 */
static void bsp_encoder_dma_done(
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    en_core_i2c_status_t  status,
    void                 *p_user_data)
{
    bsp_encoder_driver_t *p_driver;
    uint32_t              irq_state;

    (void)port;
    (void)device_address_7bit;

    p_driver = (bsp_encoder_driver_t *)p_user_data;
    if (NULL == p_driver)
    {
        return;
    }


    (void)platform_critical_enter(&irq_state);
    if (CORE_I2C_OK == status)
    {
        (void)ab_buffer_commit_write(&p_driver->s_angle_ab, 2U);
    }
    else
    {
        (void)ab_buffer_abort_write(&p_driver->s_angle_ab);
    }
    p_driver->u8_dma_busy = 0U;
    (void)platform_critical_exit(irq_state);
}
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 检查并直接挂载 I2C 和延时接口。
 * @param[out] p_driver : Driver 对象。
 * @param[in] p_iic_interface : 同步 I2C 接口及端口配置。
 * @param[in] pf_delay_ms : 毫秒延时接口。
 * @return bsp_encoder_status_t : 挂载结果。
 */
bsp_encoder_status_t bsp_encoder_driver_inst(
    bsp_encoder_driver_t *p_driver,
    const bsp_encoder_iic_interface_t *p_iic_interface,
    bsp_encoder_delay_ms_t pf_delay_ms)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_iic_interface) ||
        (NULL == pf_delay_ms))
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    if ((NULL == p_iic_interface->pf_mem_read) ||
        (NULL == p_iic_interface->pf_mem_write) ||
        (CORE_I2C_PORT_MAX <= p_iic_interface->port))
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    if (AB_BUFFER_OK != ab_buffer_init(&p_driver->s_angle_ab,
                                       p_driver->a_u8_dma_buf_a,
                                       p_driver->a_u8_dma_buf_b,
                                       2U))
    {
        return BSP_ENCODER_ERR_STATE;
    }

    p_driver->iic_interface = *p_iic_interface;
    p_driver->pf_delay_ms = pf_delay_ms;
    p_driver->u8_dma_enable = 0U;
    p_driver->u8_dma_busy = 0U;
    p_driver->u16_latest_angle = 0U;
    return BSP_ENCODER_OK;
}

/**
 * @brief 读取处理后角度的 12 位计数。
 * @param[in] p_driver : Driver 对象。
 * @param[out] p_u16_angle : 处理后的角度计数。
 * @return bsp_encoder_status_t : 读取结果。
 */
bsp_encoder_status_t bsp_encoder_read_angle(
    const bsp_encoder_driver_t *p_driver, uint16_t *p_u16_angle)
{
    uint16_t u16_value = 0U;
    bsp_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_u16_angle)
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_encoder_read_word(p_driver, ENCODER_REG_ANGLE, &u16_value);
    if (BSP_ENCODER_OK == status)
    {
        *p_u16_angle = (uint16_t)(u16_value & ENCODER_ANGLE_MASK);
    }
    return status;
}

/**
 * @brief 读取原始角度的 12 位计数。
 * @param[in] p_driver : Driver 对象。
 * @param[out] p_u16_angle : 原始角度计数。
 * @return bsp_encoder_status_t : 读取结果。
 */
bsp_encoder_status_t bsp_encoder_read_raw_angle(
    const bsp_encoder_driver_t *p_driver, uint16_t *p_u16_angle)
{
    uint16_t u16_value = 0U;
    bsp_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_u16_angle)
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_encoder_read_word(p_driver, ENCODER_REG_RAW_ANGLE,
                                  &u16_value);
    if (BSP_ENCODER_OK == status)
    {
        *p_u16_angle = (uint16_t)(u16_value & ENCODER_ANGLE_MASK);
    }
    return status;
}

/**
 * @brief 启动一次角度 DMA 接收，由外部采样节拍反复调用。
 *
 * @param[in,out] p_driver : Driver 对象。
 *
 * @return bsp_encoder_status_t : 启动结果；上一笔在飞时返回 BUSY。
 * @note 本函数不再由完成回调自动续传，固定采样由 HRTIM 分频触发。
 */
bsp_encoder_status_t bsp_encoder_start_angle_dma(
    bsp_encoder_driver_t *p_driver)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_driver)
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    if (NULL == p_driver->iic_interface.pf_submit)
    {
        return BSP_ENCODER_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    p_driver->u8_dma_enable = 1U;
    return bsp_encoder_start_dma_once(p_driver);
}

/**
 * @brief 停止角度连续 DMA 接收。
 *
 * @param[in,out] p_driver : Driver 对象。
 *
 * @return bsp_encoder_status_t : 停止结果。
 */
bsp_encoder_status_t bsp_encoder_stop_angle_dma(
    bsp_encoder_driver_t *p_driver)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_driver)
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    p_driver->u8_dma_enable = 0U;
    return BSP_ENCODER_OK;
}

/**
 * @brief 从 AB 双缓冲取得最新角度计数。
 *
 * @param[in,out] p_driver : Driver 对象。
 * @param[out] p_u16_angle : 12 位角度计数。
 *
 * @return bsp_encoder_status_t : 成功、空或参数错误。
 */
bsp_encoder_status_t bsp_encoder_get_angle_dma(
    bsp_encoder_driver_t *p_driver, uint16_t *p_u16_angle)
{
    const uint8_t       *p_data = NULL;
    size_t               length = 0U;
    uint16_t             u16_angle = 0U;
    uint8_t              found = 0U;
    ab_buffer_status_t   ab_status;
    uint32_t             irq_state;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_u16_angle))
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    for (;;)
    {
        (void)platform_critical_enter(&irq_state);
        ab_status = ab_buffer_acquire_read(&p_driver->s_angle_ab,
                                           &p_data, &length);
        (void)platform_critical_exit(irq_state);

        if (AB_BUFFER_OK != ab_status)
        {
            break;
        }

        if (length >= 2U)
        {
            u16_angle = (uint16_t)(((uint16_t)p_data[0] << 8U) |
                                    (uint16_t)p_data[1]);
            u16_angle &= ENCODER_ANGLE_MASK;
            found = 1U;
        }

        (void)platform_critical_enter(&irq_state);
        (void)ab_buffer_release_read(&p_driver->s_angle_ab);
        (void)platform_critical_exit(irq_state);
    }

    if (0U == found)
    {
        return BSP_ENCODER_ERR_EMPTY;
    }

    p_driver->u16_latest_angle = u16_angle;
    *p_u16_angle = u16_angle;
    return BSP_ENCODER_OK;
}

/**
 * @brief 设置起止位置并清零 MANG，支持跨越角度零点的范围。
 * @param[in] p_driver : Driver 对象。
 * @param[in] u16_start : 起始原始角度计数。
 * @param[in] u16_stop : 终止原始角度计数。
 * @return bsp_encoder_status_t : 设置结果。
 */
bsp_encoder_status_t bsp_encoder_set_range(
    const bsp_encoder_driver_t *p_driver,
    uint16_t u16_start, uint16_t u16_stop)
{
    uint16_t u16_span;
    bsp_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((BSP_ENCODER_ANGLE_MAX < u16_start) ||
        (BSP_ENCODER_ANGLE_MAX < u16_stop))
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    u16_span = (uint16_t)(((uint32_t)u16_stop +
                          ENCODER_COUNTS_PER_TURN - u16_start) &
                         ENCODER_ANGLE_MASK);
    if ((0U != u16_span) && (ENCODER_MIN_RANGE > u16_span))
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_encoder_update_word(p_driver, ENCODER_REG_ZPOS,
                                     ENCODER_ANGLE_MASK, u16_start);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    status = bsp_encoder_update_word(p_driver, ENCODER_REG_MPOS,
                                     ENCODER_ANGLE_MASK, u16_stop);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    return bsp_encoder_update_word(p_driver, ENCODER_REG_MANG,
                                   ENCODER_ANGLE_MASK, 0U);
}

/**
 * @brief 仅更新 CONF 的 PM 和 WD 位，保留其他设置。
 * @param[in] p_driver : Driver 对象。
 * @param[in] mode : 电源模式。
 * @param[in] watchdog_enabled : 低功耗看门狗使能。
 * @return bsp_encoder_status_t : 设置结果。
 */
bsp_encoder_status_t bsp_encoder_set_power_mode(
    const bsp_encoder_driver_t *p_driver,
    bsp_encoder_power_mode_t mode, bool watchdog_enabled)
{
    uint16_t u16_value;

    //-------------------------参数检查 begin-------------------------//
    if ((unsigned int)BSP_ENCODER_POWER_LPM3 < (unsigned int)mode)
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    u16_value = (uint16_t)mode;
    if (true == watchdog_enabled)
    {
        u16_value = (uint16_t)(u16_value | ENCODER_CONF_WD_MASK);
    }
    return bsp_encoder_update_word(p_driver, ENCODER_REG_CONF,
        (uint16_t)(ENCODER_CONF_PM_MASK | ENCODER_CONF_WD_MASK), u16_value);
}

/**
 * @brief 设置片内慢滤波强度、快滤波阈值和迟滞，保留 CONF 其他位。
 * @param[in] p_driver : 已挂载接口的 Driver 对象。
 * @param[in] slow_filter : 慢滤波强度，16x/8x/4x/2x。
 * @param[in] fast_filter : 快滤波阈值，SLOW_ONLY 表示只用慢滤波。
 * @param[in] hysteresis : 输出迟滞，0~3 LSB。
 * @return bsp_encoder_status_t : 参数、接口状态或传输结果。
 * @note 只修改易失 CONF 寄存器，掉电恢复默认；
 *       需永久生效须调用 bsp_encoder_save_config(BSP_ENCODER_SAVE_SETTING)。
 *       低功耗模式下慢滤波被芯片强制，SF 设置可能不生效。
 */
bsp_encoder_status_t bsp_encoder_set_filter(
    const bsp_encoder_driver_t *p_driver,
    bsp_encoder_slow_filter_t slow_filter,
    bsp_encoder_fast_filter_t fast_filter,
    bsp_encoder_hysteresis_t hysteresis)
{
    uint16_t u16_value;

    //-------------------------参数检查 begin-------------------------//
    if (((unsigned int)BSP_ENCODER_SLOW_FILTER_2X <
         (unsigned int)slow_filter) ||
        ((unsigned int)BSP_ENCODER_FAST_FILTER_10LSB <
         (unsigned int)fast_filter) ||
        ((unsigned int)BSP_ENCODER_HYSTERESIS_3LSB <
         (unsigned int)hysteresis))
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    u16_value = (uint16_t)(
        ((uint16_t)slow_filter << ENCODER_CONF_SF_SHIFT) |
        ((uint16_t)fast_filter << ENCODER_CONF_FTH_SHIFT) |
        ((uint16_t)hysteresis << ENCODER_CONF_HYST_SHIFT));

    return bsp_encoder_update_word(p_driver, ENCODER_REG_CONF,
        (uint16_t)(ENCODER_CONF_SF_MASK | ENCODER_CONF_FTH_MASK |
                   ENCODER_CONF_HYST_MASK), u16_value);
}

/**
 * @brief 检查条件后执行一次烧录，重载 OTP 并核对目标寄存器。
 * @param[in] p_driver : Driver 对象。
 * @param[in] target : 角度范围或配置保存区域。
 * @return bsp_encoder_status_t : 操作结果；发出烧录后失败不可自动重试。
 */
bsp_encoder_status_t bsp_encoder_save_config(
    const bsp_encoder_driver_t *p_driver, bsp_encoder_save_t target)
{
    uint8_t u8_zmco = 0U;
    uint8_t u8_magnet_status = 0U;
    uint8_t u8_command;
    uint8_t u8_register;
    uint8_t a_u8_expected[4] = {0U, 0U, 0U, 0U};
    uint8_t a_u8_actual[4] = {0U, 0U, 0U, 0U};
    uint8_t u8_index;
    const uint8_t a_u8_reload[3] = {0x01U, 0x11U, 0x10U};
    bsp_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((unsigned int)BSP_ENCODER_SAVE_SETTING < (unsigned int)target)
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    status = bsp_encoder_check(p_driver);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_encoder_iic_read(p_driver, ENCODER_REG_ZMCO, &u8_zmco, 1U);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    u8_zmco &= ENCODER_ZMCO_MASK;
    if (BSP_ENCODER_SAVE_ANGLE == target)
    {
        if (3U <= u8_zmco)
        {
            return BSP_ENCODER_ERR_STATE;
        }
        status = bsp_encoder_iic_read(p_driver, ENCODER_REG_STATUS,
                                      &u8_magnet_status, 1U);
        if (BSP_ENCODER_OK != status)
        {
            return status;
        }
        if (0U == (u8_magnet_status & ENCODER_STATUS_MD_MASK))
        {
            return BSP_ENCODER_ERR_STATE;
        }
        u8_register = ENCODER_REG_ZPOS;
        u8_command = ENCODER_BURN_ANGLE;
    }
    else
    {
        u8_register = ENCODER_REG_MANG;
        u8_command = ENCODER_BURN_SETTING;
    }

    status = bsp_encoder_iic_read(p_driver, u8_register, a_u8_expected, 4U);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    if ((BSP_ENCODER_SAVE_SETTING == target) && (0U != u8_zmco) &&
        ((0U != (a_u8_expected[0] & 0x0FU)) ||
         (0U != a_u8_expected[1])))
    {
        return BSP_ENCODER_ERR_STATE;
    }

    /* 此处开始会消耗 OTP 次数；后续任何失败均直接交由调用者处理。 */
    status = bsp_encoder_iic_write(p_driver, ENCODER_REG_BURN,
                                   &u8_command, 1U);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    status = p_driver->pf_delay_ms(1U);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }
    for (u8_index = 0U; u8_index < 3U; u8_index++)
    {
        status = bsp_encoder_iic_write(p_driver, ENCODER_REG_BURN,
                                       &a_u8_reload[u8_index], 1U);
        if (BSP_ENCODER_OK != status)
        {
            return status;
        }
    }
    status = bsp_encoder_iic_read(p_driver, u8_register, a_u8_actual, 4U);
    if (BSP_ENCODER_OK != status)
    {
        return status;
    }

    /* 只核对已定义位，不比较芯片保留位。 */
    a_u8_expected[0] &= 0x0FU;
    a_u8_actual[0] &= 0x0FU;
    if (BSP_ENCODER_SAVE_ANGLE == target)
    {
        a_u8_expected[2] &= 0x0FU;
        a_u8_actual[2] &= 0x0FU;
    }
    else
    {
        a_u8_expected[2] &= 0x3FU;
        a_u8_actual[2] &= 0x3FU;
    }
    for (u8_index = 0U; u8_index < 4U; u8_index++)
    {
        if (a_u8_expected[u8_index] != a_u8_actual[u8_index])
        {
            return BSP_ENCODER_ERR_VERIFY;
        }
    }
    return BSP_ENCODER_OK;
}
//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
