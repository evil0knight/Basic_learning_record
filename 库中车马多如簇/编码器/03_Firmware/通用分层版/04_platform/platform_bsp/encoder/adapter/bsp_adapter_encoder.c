/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_adapter_encoder.c
 *
 * @par dependencies
 * - bsp_adapter_encoder.h
 * - ../wrapper/bsp_wrapper_encoder.h
 * - ../../../../05_bsp/bsp_encoder/handler/bsp_encoder_handler.h
 * - ../../../platform_mcu/i2c/port/bare/i2c_port_bare.h
 * - ../../../platform_mcu/time/inc/time_port.h
 * - stddef.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 组装编码器的平台 I2C 与延时资源，把 Handler 挂载到 Wrapper。
 *
 * Processing flow:
 *
 * 接口配置表 -> Handler inst -> 每个操作函数直调 Handler control。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "bsp_adapter_encoder.h"

#include "../wrapper/bsp_wrapper_encoder.h"
#include "../../../../05_bsp/bsp_encoder/handler/bsp_encoder_handler.h"
#include "../../../platform_mcu/i2c/port/bare/i2c_port_bare.h"
#include "../../../platform_mcu/time/inc/time_port.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//******************************* Defines begin ******************************//

/* 单次 I2C 传输超时，单位 ms。 */
#ifndef BSP_ADAPTER_ENCODER_I2C_TIMEOUT_MS
#define BSP_ADAPTER_ENCODER_I2C_TIMEOUT_MS (10U)
#endif

//******************************** Defines end *******************************//

//************************ Function Prototypes begin *************************//
static bsp_encoder_status_t bsp_adapter_encoder_delay_ms(uint32_t u32_ms);
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_status_convert(
    bsp_encoder_status_t status);
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_init(
    bsp_wrapper_encoder_dev_t *p_dev);
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_read_angle(
    bsp_wrapper_encoder_dev_t *p_dev,
    uint16_t *p_u16_angle);
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_read_raw_angle(
    bsp_wrapper_encoder_dev_t *p_dev,
    uint16_t *p_u16_raw_angle);
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_set_range(
    bsp_wrapper_encoder_dev_t *p_dev,
    const bsp_encoder_range_t *p_range);
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_set_power_mode(
    bsp_wrapper_encoder_dev_t *p_dev,
    const bsp_encoder_power_config_t *p_config);
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_set_filter(
    bsp_wrapper_encoder_dev_t *p_dev,
    const bsp_encoder_filter_t *p_filter);
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_save_config(
    bsp_wrapper_encoder_dev_t *p_dev,
    bsp_encoder_save_t target);
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_start_dma(
    bsp_wrapper_encoder_dev_t *p_dev);
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_stop_dma(
    bsp_wrapper_encoder_dev_t *p_dev);
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_get_dma_angle(
    bsp_wrapper_encoder_dev_t *p_dev,
    uint16_t *p_u16_angle);
//************************* Function Prototypes end **************************//

//************************** Static Variables begin **************************//
/* 每台编码器的 I2C 接口配置：逻辑槽位、超时和平台读写函数。 */
static const bsp_encoder_iic_interface_t
    g_a_bsp_adapter_encoder_iic[BSP_ENCODER_ID_MAX] =
{
    [BSP_ENCODER_ID_MOTOR] =
    {
        .pf_mem_read = core_i2c_mem_read,
        .pf_mem_write = core_i2c_mem_write,
        .pf_submit = core_i2c_submit,
        .port = CORE_I2C_PORT_ENCODER1,
        .u32_timeout_ms = BSP_ADAPTER_ENCODER_I2C_TIMEOUT_MS
    }
};

/* 每台编码器的 Driver 实例。 */
static bsp_encoder_driver_t
    g_a_bsp_adapter_encoder_driver[BSP_ENCODER_ID_MAX];

/* 唯一的 Handler 实例，统一管理全部编码器 Driver。 */
static bsp_encoder_handler_t g_bsp_adapter_encoder_handler;
//*************************** Static Variables end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 将编码器 Adapter 注册到 BSP Wrapper。
 *
 * Steps:
 *  1. 按编码器编号逐台组装 {u32_idx, p_user_data, 7 个操作函数}。
 *  2. 逐台调用 bsp_wrapper_encoder_reg() 挂载函数表。
 *
 * @return void : 无返回值；注册结果由 Wrapper 内部记录。
 * @note 需在 Wrapper 首次被调用前注册一次，重复注册会被 Wrapper 拒绝。
 *       注册成功后由 bsp_wrapper_encoder_init() 触发 Handler 挂载。
 *
 */
void bsp_adapter_encoder_register(void)
{
    uint32_t u32_index;

    for (u32_index = 0U; u32_index < BSP_ENCODER_ID_MAX; u32_index++)
    {
        bsp_wrapper_encoder_dev_t dev =
        {
            .u32_idx = u32_index,
            .p_user_data = NULL,
            .init = bsp_adapter_encoder_init,
            .read_angle = bsp_adapter_encoder_read_angle,
            .read_raw_angle = bsp_adapter_encoder_read_raw_angle,
            .set_range = bsp_adapter_encoder_set_range,
            .set_power_mode = bsp_adapter_encoder_set_power_mode,
            .set_filter = bsp_adapter_encoder_set_filter,
            .save_config = bsp_adapter_encoder_save_config,
            .start_dma = bsp_adapter_encoder_start_dma,
            .stop_dma = bsp_adapter_encoder_stop_dma,
            .get_dma_angle = bsp_adapter_encoder_get_dma_angle
        };

        (void)bsp_wrapper_encoder_reg(u32_index, &dev);
    }
}
//************************* Function Definitions end *************************//

//************************** Static Functions begin **************************//
/**
 * @brief 基于毫秒时间基准的阻塞延时，供 Driver 等待配置生效。
 *
 * Steps:
 *  1. 检查延时值，0 ms 直接返回。
 *  2. 记录起始毫秒计数并忙等到达目标时间。
 *
 * @param[in] u32_ms : 延时时间，单位 ms。
 *
 * @return bsp_encoder_status_t : 固定返回 BSP_ENCODER_OK。
 *
 */
static bsp_encoder_status_t bsp_adapter_encoder_delay_ms(uint32_t u32_ms)
{
    uint32_t u32_start;

    //-------------------------参数检查 begin-------------------------//
    if (0U == u32_ms)
    {
        return BSP_ENCODER_OK;
    }
    //--------------------------参数检查 end--------------------------//

    u32_start = core_time_get();
    while (u32_ms > (uint32_t)(core_time_get() - u32_start))
    {
    }

    return BSP_ENCODER_OK;
}

/**
 * @brief 把 Handler 状态码转换为 Wrapper 状态码。
 *
 * Steps:
 *  1. 按状态枚举逐项映射。
 *  2. 未列出的状态统一归入 BSP_WRAPPER_ENCODER_ERROR。
 *
 * @param[in] status : Handler/Driver 侧返回的状态。
 *
 * @return bsp_wrapper_encoder_status_t : 对应的 Wrapper 状态。
 *
 */
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_status_convert(
    bsp_encoder_status_t status)
{
    switch (status)
    {
        case BSP_ENCODER_OK:
            return BSP_WRAPPER_ENCODER_OK;
        case BSP_ENCODER_ERR_ARGUMENT:
            return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
        case BSP_ENCODER_ERR_STATE:
            return BSP_WRAPPER_ENCODER_ERR_STATE;
        case BSP_ENCODER_ERR_TIMEOUT:
            return BSP_WRAPPER_ENCODER_ERR_TIMEOUT;
        case BSP_ENCODER_ERR_BUSY:
            return BSP_WRAPPER_ENCODER_ERR_BUSY;
        case BSP_ENCODER_ERR_NACK:
            return BSP_WRAPPER_ENCODER_ERR_NACK;
        default:
            return BSP_WRAPPER_ENCODER_ERROR;
    }
}

/**
 * @brief 用平台资源初始化编码器 Handler 与全部 Driver。
 *
 * Steps:
 *  1. 把 Driver 数组、接口配置和延时接口传给 Handler inst。
 *  2. Handler inst 内部逐个调用 Driver inst 挂载驱动接口。
 *
 * @param[in] p_dev : Wrapper 透传的器件函数表，本函数不使用。
 *
 * @return bsp_wrapper_encoder_status_t : Handler 初始化结果。
 * @note 只挂载接口，不访问 AS5600，不修改芯片寄存器。
 *
 */
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_init(
    bsp_wrapper_encoder_dev_t *p_dev)
{
    (void)p_dev;

    return bsp_adapter_encoder_status_convert(
        bsp_encoder_handler_inst(&g_bsp_adapter_encoder_handler,
                                 g_a_bsp_adapter_encoder_driver,
                                 (uint32_t)BSP_ENCODER_ID_MAX,
                                 g_a_bsp_adapter_encoder_iic,
                                 bsp_adapter_encoder_delay_ms));
}

/**
 * @brief 读取指定编码器经过范围缩放和滤波的 ANGLE 值。
 *
 * Steps:
 *  1. 检查器件函数表和输出指针是否有效。
 *  2. 以 BSP_ENCODER_CMD_READ_ANGLE 直调 Handler control。
 *
 * @param[in] p_dev : Wrapper 透传的器件函数表，u32_idx 即 Handler 器件索引。
 * @param[out] p_u16_angle : 12 位角度计数，范围 0~4095。
 *
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 *
 */
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_read_angle(
    bsp_wrapper_encoder_dev_t *p_dev,
    uint16_t *p_u16_angle)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_dev) || (NULL == p_u16_angle))
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return bsp_adapter_encoder_status_convert(
        bsp_encoder_handler_control(&g_bsp_adapter_encoder_handler,
                                    p_dev->u32_idx,
                                    (uint32_t)BSP_ENCODER_CMD_READ_ANGLE,
                                    p_u16_angle));
}

/**
 * @brief 读取指定编码器未经范围缩放的 RAW ANGLE 值。
 *
 * Steps:
 *  1. 检查器件函数表和输出指针是否有效。
 *  2. 以 BSP_ENCODER_CMD_READ_RAW_ANGLE 直调 Handler control。
 *
 * @param[in] p_dev : Wrapper 透传的器件函数表。
 * @param[out] p_u16_raw_angle : 0~4095 的原始角度计数。
 *
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 *
 */
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_read_raw_angle(
    bsp_wrapper_encoder_dev_t *p_dev,
    uint16_t *p_u16_raw_angle)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_dev) || (NULL == p_u16_raw_angle))
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return bsp_adapter_encoder_status_convert(
        bsp_encoder_handler_control(&g_bsp_adapter_encoder_handler,
                                    p_dev->u32_idx,
                                    (uint32_t)BSP_ENCODER_CMD_READ_RAW_ANGLE,
                                    p_u16_raw_angle));
}

/**
 * @brief 设置指定编码器的 ZPOS/MPOS 测量起止范围。
 *
 * Steps:
 *  1. 检查器件函数表和范围指针是否有效。
 *  2. 复制一份参数后以 BSP_ENCODER_CMD_SET_RANGE 直调 Handler control。
 *
 * @param[in] p_dev : Wrapper 透传的器件函数表。
 * @param[in] p_range : 起止原始角度计数，各为 0~4095。
 *
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 *
 */
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_set_range(
    bsp_wrapper_encoder_dev_t *p_dev,
    const bsp_encoder_range_t *p_range)
{
    bsp_encoder_range_t range;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_dev) || (NULL == p_range))
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    range = *p_range;
    return bsp_adapter_encoder_status_convert(
        bsp_encoder_handler_control(&g_bsp_adapter_encoder_handler,
                                    p_dev->u32_idx,
                                    (uint32_t)BSP_ENCODER_CMD_SET_RANGE,
                                    &range));
}

/**
 * @brief 设置指定编码器的电源模式和低功耗看门狗开关。
 *
 * Steps:
 *  1. 检查器件函数表和配置指针是否有效。
 *  2. 复制一份参数后以 BSP_ENCODER_CMD_SET_POWER_MODE 直调 Handler control。
 *
 * @param[in] p_dev : Wrapper 透传的器件函数表。
 * @param[in] p_config : 电源模式与看门狗使能配置。
 *
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 *
 */
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_set_power_mode(
    bsp_wrapper_encoder_dev_t *p_dev,
    const bsp_encoder_power_config_t *p_config)
{
    bsp_encoder_power_config_t config;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_dev) || (NULL == p_config))
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    config = *p_config;
    return bsp_adapter_encoder_status_convert(
        bsp_encoder_handler_control(&g_bsp_adapter_encoder_handler,
                                    p_dev->u32_idx,
                                    (uint32_t)BSP_ENCODER_CMD_SET_POWER_MODE,
                                    &config));
}

/**
 * @brief 设置指定编码器的慢滤波强度、快滤波阈值和迟滞。
 *
 * Steps:
 *  1. 检查器件函数表和配置指针是否有效。
 *  2. 复制一份参数后以 BSP_ENCODER_CMD_SET_FILTER 直调 Handler control。
 *
 * @param[in] p_dev : Wrapper 透传的器件函数表。
 * @param[in] p_filter : 慢滤波、快滤波阈值与迟滞配置。
 *
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 * @note 只写易失 CONF，掉电恢复默认；永久生效需烧 SETTING。
 *
 */
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_set_filter(
    bsp_wrapper_encoder_dev_t *p_dev,
    const bsp_encoder_filter_t *p_filter)
{
    bsp_encoder_filter_t filter;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_dev) || (NULL == p_filter))
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    filter = *p_filter;
    return bsp_adapter_encoder_status_convert(
        bsp_encoder_handler_control(&g_bsp_adapter_encoder_handler,
                                    p_dev->u32_idx,
                                    (uint32_t)BSP_ENCODER_CMD_SET_FILTER,
                                    &filter));
}

/**
 * @brief 烧录指定编码器的选定 OTP 区域。
 *
 * Steps:
 *  1. 检查器件函数表是否有效。
 *  2. 以 BSP_ENCODER_CMD_SAVE_CONFIG 直调 Handler control。
 *
 * @param[in] p_dev : Wrapper 透传的器件函数表。
 * @param[in] target : ANGLE 保存 ZPOS/MPOS，SETTING 保存 MANG/CONF。
 *
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 * @note 烧录有次数限制且不可自动重试，调用前须确认供电与烧录条件。
 *
 */
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_save_config(
    bsp_wrapper_encoder_dev_t *p_dev,
    bsp_encoder_save_t target)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_dev)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return bsp_adapter_encoder_status_convert(
        bsp_encoder_handler_control(&g_bsp_adapter_encoder_handler,
                                    p_dev->u32_idx,
                                    (uint32_t)BSP_ENCODER_CMD_SAVE_CONFIG,
                                    &target));
}

/**
 * @brief 启动指定编码器的一笔异步角度 DMA 采样。
 *
 * @param[in] p_dev : Wrapper 透传的器件函数表。
 *
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 */
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_start_dma(
    bsp_wrapper_encoder_dev_t *p_dev)
{
    if (NULL == p_dev)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }

    return bsp_adapter_encoder_status_convert(
        bsp_encoder_handler_control(&g_bsp_adapter_encoder_handler,
                                    p_dev->u32_idx,
                                    (uint32_t)BSP_ENCODER_CMD_START_DMA,
                                    p_dev));
}

/**
 * @brief 停止指定编码器的异步角度 DMA 采样。
 *
 * @param[in] p_dev : Wrapper 透传的器件函数表。
 *
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 */
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_stop_dma(
    bsp_wrapper_encoder_dev_t *p_dev)
{
    if (NULL == p_dev)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }

    return bsp_adapter_encoder_status_convert(
        bsp_encoder_handler_control(&g_bsp_adapter_encoder_handler,
                                    p_dev->u32_idx,
                                    (uint32_t)BSP_ENCODER_CMD_STOP_DMA,
                                    p_dev));
}

/**
 * @brief 从 AB 双缓冲取得指定编码器的最新角度计数。
 *
 * @param[in] p_dev : Wrapper 透传的器件函数表。
 * @param[out] p_u16_angle : 12 位角度计数。
 *
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 */
static bsp_wrapper_encoder_status_t bsp_adapter_encoder_get_dma_angle(
    bsp_wrapper_encoder_dev_t *p_dev,
    uint16_t *p_u16_angle)
{
    if ((NULL == p_dev) || (NULL == p_u16_angle))
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }

    return bsp_adapter_encoder_status_convert(
        bsp_encoder_handler_control(&g_bsp_adapter_encoder_handler,
                                    p_dev->u32_idx,
                                    (uint32_t)BSP_ENCODER_CMD_GET_DMA_ANGLE,
                                    p_u16_angle));
}

//*************************** Static Functions end ***************************//

//************************ Compile-time Checks begin *************************//
/* 板级编码器编号数量不能超过 Wrapper 的索引上限。 */
typedef char bsp_adapter_encoder_id_check_t[
    (BSP_ENCODER_ID_MAX <= BSP_WRAPPER_ENCODER_DRV_MAX_NUM) ? 1 : -1];
//************************* Compile-time Checks end **************************//

//******************************* End of File ********************************//
