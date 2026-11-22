/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_wrapper_encoder.c
 *
 * @par dependencies
 *
 * @author YA
 *
 * @brief 编码器硬件隔离层：通过 Platform 单路 I2C 实现采样与配置。
 *
 * Processing flow:
 *
 * Service 调用 -> Wrapper 调用 Platform core_iic_* -> 输出采样结果。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "bsp_wrapper_encoder.h"
#include "bsp_wrapper_encoder_reg.h"
#include "../../../platform_mcu/i2c/inc/core_iic.h"
#include "../../../platform_mcu/time/inc/time_port.h"
#include "../../../../03_config/feature_config.h"
#if (((CFG_FEATURE_MEASURE_ENCODER_TIME > 0U) || (CFG_FEATURE_MEASURE_ENCODER_DMA_TIME > 0U)))
#include "../../../../06_middlewares/debug/dwt_measure.h"
#endif
//******************************* Includes end *******************************//

/* 12 位计数转换为机械弧度及微秒转换为秒的系数。 */
#define BSP_WRAPPER_ENCODER_COUNTS_PER_TURN (4096)                /* 每圈计数 */
#define BSP_WRAPPER_ENCODER_HALF_TURN       (2048)                /* 半圈计数 */
#define BSP_WRAPPER_ENCODER_RAD_PER_COUNT (6.28318530718f/4096.0f) /*弧度/计数*/
#define BSP_WRAPPER_ENCODER_US_PER_SECOND (1000000.0f)          /*秒->微秒转换*/
#define BSP_WRAPPER_ENCODER_DEG_PER_COUNT (360.0f/4096.0f)         /* 度/计数 */

//************************** Static Variables begin **************************//
static float g_f_latest_angle_deg;
static float g_f_latest_angle_velocity;
static uint8_t g_u8_velocity_has_previous;
static struct
{
    float f_average_velocity_rad_s; /* 最近一次平均机械角速度/rad/s。 */
    float f_filtered_velocity_rad_s; /* 供速度环使用的低通角速度。 */
    uint32_t u32_velocity_filter_time_us; /* 上次有效滤波时间。 */
    uint8_t u8_velocity_filter_valid; /* 首次有效值直接初始化。 */
    float f_velocity_sum;           /* 新样本角速度累计值。 */
    uint32_t u32_velocity_count;    /* 累计速度样本数。 */
    uint8_t a_u8_dma_rx[2]; /* DMA 接收区，与已发布角度分离。 */
    volatile uint16_t u16_pending_angle;
    volatile uint32_t u32_pending_time_us; /* DMA 完成时刻。 */
    volatile uint8_t u8_sample_ready;
    volatile uint8_t u8_dma_enable;
    volatile uint8_t u8_dma_busy;
    uint32_t u32_dma_start_ms;
    volatile uint16_t u16_latest_angle;
    uint16_t u16_last_angle_count;  /* 上次采样计数，UINT16_MAX 表示无历史。 */
    uint32_t u32_last_time_us;      /* 上次采样时间戳，单位 us。 */
    uint32_t u32_latest_time_us;    /* 当前采样时间戳，单位 us。 */
} g_encoder;

/** @brief 计算 DWT 微秒时间戳差值，支持 32 位计数器回绕。 */
//*************************** Static Variables end ***************************//

/** @brief 将 Platform I2C 状态映射为编码器状态。 */
static bsp_wrapper_encoder_status_t bsp_wrapper_encoder_iic_status(
    en_core_iic_status_t en_status)
{
    switch (en_status)
    {
        case CORE_IIC_OK:
            return BSP_WRAPPER_ENCODER_OK;

        case CORE_IIC_ERR_ARGUMENT:
            return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;

        case CORE_IIC_ERR_BUSY:
            return BSP_WRAPPER_ENCODER_ERR_BUSY;

        case CORE_IIC_ERR_TIMEOUT:
            return BSP_WRAPPER_ENCODER_ERR_TIMEOUT;

        case CORE_IIC_ERR_NACK:
            return BSP_WRAPPER_ENCODER_ERR_NACK;

        default:
            return BSP_WRAPPER_ENCODER_ERROR;
    }
}

/**
 * @brief 完成时发布完整角度，失败或中止不覆盖未消费的有效样本。
 * @param[in] success : 本次传输是否成功。
 * @return bsp_wrapper_encoder_status_t : 返回成功。
 * @note 与 ADC 消费者同抢占优先级；只保留最新完成样本。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_dma_done(bool success)
{
    if (0U != g_encoder.u8_dma_busy)
    {
        if (success && (1U == g_encoder.u8_dma_busy))
        {
            g_encoder.u16_pending_angle = (uint16_t)(
                (((uint16_t)g_encoder.a_u8_dma_rx[0] << 8U) |
                 (uint16_t)g_encoder.a_u8_dma_rx[1]) & ENCODER_ANGLE_MASK);
            g_encoder.u32_pending_time_us = core_time_get_us();
            g_encoder.u8_sample_ready = 1U;
#if (((CFG_FEATURE_MEASURE_ENCODER_TIME > 0U) || (CFG_FEATURE_MEASURE_ENCODER_DMA_TIME > 0U)))
            /* 仅统计成功且未处于中止状态的传输。
             * 先发布数据和时间戳，再统计，避免打印影响样本时间戳。 */
            (void)dwt_measure_exit();
#endif
        }
        g_encoder.u8_dma_busy = 0U;
    }
    return BSP_WRAPPER_ENCODER_OK;
}
/** @brief 初始化接收缓存及角度、速度历史；不得在传输中重置。 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_init(void)
{
    if (0U != g_encoder.u8_dma_busy) { return BSP_WRAPPER_ENCODER_ERR_BUSY; }
    g_encoder.u16_pending_angle = 0U;
    g_encoder.u8_sample_ready = 0U;
    g_encoder.u8_dma_enable = 0U;
    g_encoder.u16_latest_angle = UINT16_MAX;
    g_encoder.u16_last_angle_count = UINT16_MAX;
    g_encoder.u32_last_time_us = 0U;
    g_encoder.u32_latest_time_us = 0U;
    g_f_latest_angle_deg = 0.0f;
    g_f_latest_angle_velocity = 0.0f;
    g_encoder.f_average_velocity_rad_s = 0.0f;
    g_encoder.f_filtered_velocity_rad_s = 0.0f;
    g_encoder.u32_velocity_filter_time_us = 0U;
    g_encoder.u8_velocity_filter_valid = 0U;
    g_u8_velocity_has_previous = 0U;
    g_encoder.f_velocity_sum = 0.0f;
    g_encoder.u32_velocity_count = 0U;
    return BSP_WRAPPER_ENCODER_OK;
}
/**
 * @brief 堵塞读取处理后角度的 12 位计数。
 * @param[out] p_u16_angle : 处理后的角度计数。
 * @return bsp_wrapper_encoder_status_t : 读取结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_read_angle(
    uint16_t *p_u16_angle)
{
    uint16_t u16_value = 0U;
    bsp_wrapper_encoder_status_t status;

    if (NULL == p_u16_angle) { return BSP_WRAPPER_ENCODER_ERR_ARGUMENT; }
    if (0U != g_encoder.u8_dma_busy)
    {
        return BSP_WRAPPER_ENCODER_ERR_BUSY;
    }
    status = bsp_wrapper_encoder_iic_status(core_iic_read_word(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_ANGLE, &u16_value));
    if (BSP_WRAPPER_ENCODER_OK == status)
    {
        *p_u16_angle = (uint16_t)(u16_value & ENCODER_ANGLE_MASK);
        /* 同步读也发布新反馈，供 DMA 调度暂停时准备输出。 */
        g_encoder.u16_pending_angle = *p_u16_angle;
        g_encoder.u32_pending_time_us = core_time_get_us();
        g_encoder.u8_sample_ready = 1U;
    }
    return status;
}

/**
 * @brief 读取原始角度的 12 位计数。
 * @param[out] p_u16_angle : 原始角度计数。
 * @return bsp_wrapper_encoder_status_t : 读取结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_read_raw_angle(
    uint16_t *p_u16_angle)
{
    uint16_t u16_value = 0U;
    bsp_wrapper_encoder_status_t status;

    status = bsp_wrapper_encoder_iic_status(core_iic_read_word(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_RAW_ANGLE, &u16_value));
    if (BSP_WRAPPER_ENCODER_OK == status)
    {
        *p_u16_angle = (uint16_t)(u16_value & ENCODER_ANGLE_MASK);
    }
    return status;
}
/**
 * @brief 启动一次角度 DMA 接收，由外部采样节拍反复调用。
 *
 * Steps:
 *  1. 使能 DMA 采集，检查上一笔传输是否忙碌或超时。
 *  2. 忙碌时返回 BUSY，超时则请求中止。
 *  3. 空闲时启动角度 DMA，启动失败则清除忙碌标志。
 *
 * @return bsp_wrapper_encoder_status_t : 启动结果；上一笔在飞时返回 BUSY。
 * @note 本函数不再由完成回调自动续传，固定采样由 HRTIM 分频触发。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_start_angle_dma(
    void)
{
    /* 仅由 ADC1_2 中断调用；与 I2C1、DMA 同为抢占优先级 0。 */
    /* 同级中断不互相抢占，此处无需重复屏蔽全局中断。 */

    bsp_wrapper_encoder_status_t status;

    /* 每次外部触发均允许启动一笔采集。 */
    g_encoder.u8_dma_enable = 1U;

    if (0U != g_encoder.u8_dma_busy)
    {
        /* 中止完成前保持 busy，禁止下一笔 DMA 复用接收区。 */
        if ((1U == g_encoder.u8_dma_busy) &&
            (10U <= (uint32_t)(core_time_get() -
                               g_encoder.u32_dma_start_ms)))
        {
            g_encoder.u8_dma_busy = 2U;
            if (CORE_IIC_OK != core_iic_abort(
                    BSP_WRAPPER_ENCODER_IIC_ADDRESS))
            {
                g_encoder.u8_dma_busy = 1U;
            }
        }

        return BSP_WRAPPER_ENCODER_ERR_BUSY;
    }
    g_encoder.u8_dma_busy = 1U;

    g_encoder.u32_dma_start_ms = core_time_get();
#if (((CFG_FEATURE_MEASURE_ENCODER_TIME > 0U) || (CFG_FEATURE_MEASURE_ENCODER_DMA_TIME > 0U)))
    /* 仅在空闲且即将启动传输时记起点，BUSY 分支不覆盖起点。 */
    (void)dwt_measure_enter();
#endif
    status = bsp_wrapper_encoder_iic_status(core_iic_read_dma(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_ANGLE,
        g_encoder.a_u8_dma_rx, 2U));
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        g_encoder.u8_dma_busy = 0U;
    }

    return status;
}
/**
 * @brief 停止角度连续 DMA 接收。
 *
 *
 * @return bsp_wrapper_encoder_status_t : 停止结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_stop_angle_dma(
    void)
{
    g_encoder.u8_dma_enable = 0U;
    return BSP_WRAPPER_ENCODER_OK;
}
/**
 * @brief 消费完成回调发布的最新角度计数。
 *
 * Steps:
 *  1. 消费已提交样本并取得最新计数。
 *  2. 保存上次和当前采样的计数、时间戳，不计算角速度。
 *
 * @param[out] p_u16_angle : 12 位角度计数。
 *
 * @return bsp_wrapper_encoder_status_t : 成功、空或参数错误。
 * @note 没有新样本时返回 EMPTY，输出和当前角度缓存保持不变。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_get_angle_dma(
    uint16_t *p_u16_angle)
{
    uint32_t u32_time_us;
    uint16_t u16_angle;

    /* ADC 与发布回调同级，不会在读取与清标志之间互相抢占。 */
    if (0U == g_encoder.u8_sample_ready)
    {
        return BSP_WRAPPER_ENCODER_ERR_EMPTY;
    }
    u16_angle = g_encoder.u16_pending_angle;
    g_encoder.u8_sample_ready = 0U;
    u32_time_us = g_encoder.u32_pending_time_us;

    g_encoder.u16_last_angle_count = g_encoder.u16_latest_angle;//保存上次角度计数
    g_encoder.u32_last_time_us = g_encoder.u32_latest_time_us;//保存上次时间,供加速度
    g_encoder.u16_latest_angle = u16_angle;//保存最新角度
    g_encoder.u32_latest_time_us = u32_time_us;//当前时间

    *p_u16_angle = u16_angle;
    return BSP_WRAPPER_ENCODER_OK;
}
/**
 * @brief 新样本角速度先取均值再低通，输出后清空累计状态。
 *
 * Steps:
 *  1. 检查输出指针及累计样本数。
 *  2. 均值经2ms低通后输出，清累计值及计数，保留滤波历史。
 *
 * @param[out] p_f_velocity : 机械角速度，单位 rad/s。
 *
 * @return bsp_wrapper_encoder_status_t : 成功、参数错误或无速度样本。
 * @note 仅由同一控制中断的 1kHz 调度读取，空窗口不修改输出。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_get_angular_velocity(
    float *p_f_velocity)
{
    const float f_filter_time_s = 0.002f; /* 低通时间常数：2ms。 */
    uint32_t u32_now_us;
    float f_dt_s;
    float f_alpha;

    if (NULL == p_f_velocity)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    if (0U == g_encoder.u32_velocity_count)
    {
        return BSP_WRAPPER_ENCODER_ERR_EMPTY;
    }
    g_encoder.f_average_velocity_rad_s =
        g_encoder.f_velocity_sum / (float)g_encoder.u32_velocity_count;
    u32_now_us = core_time_get_us();
    if (0U == g_encoder.u8_velocity_filter_valid)
    {
        g_encoder.f_filtered_velocity_rad_s =
            g_encoder.f_average_velocity_rad_s;
        g_encoder.u8_velocity_filter_valid = 1U;
    }
    else
    {
        f_dt_s = (float)core_time_get_us_delta(u32_now_us,
            g_encoder.u32_velocity_filter_time_us) * 0.000001f;
        f_alpha = f_dt_s / (f_filter_time_s + f_dt_s);
        g_encoder.f_filtered_velocity_rad_s += f_alpha *
            (g_encoder.f_average_velocity_rad_s -
             g_encoder.f_filtered_velocity_rad_s);
    }
    g_encoder.u32_velocity_filter_time_us = u32_now_us;
    *p_f_velocity = g_encoder.f_filtered_velocity_rad_s;
    g_encoder.f_velocity_sum = 0.0f;
    g_encoder.u32_velocity_count = 0U;
    return BSP_WRAPPER_ENCODER_OK;
}
/**
 * @brief 设置起止位置并清零 MANG，支持跨越角度零点的范围。
 * @param[in] u16_start : 起始原始角度计数。
 * @param[in] u16_stop : 终止原始角度计数。
 * @return bsp_wrapper_encoder_status_t : 设置结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_set_range(
    uint16_t u16_start, uint16_t u16_stop)
{
    uint16_t u16_span;
    bsp_wrapper_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((BSP_WRAPPER_ENCODER_ANGLE_MAX < u16_start) ||
        (BSP_WRAPPER_ENCODER_ANGLE_MAX < u16_stop))
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    u16_span = (uint16_t)(((uint32_t)u16_stop +
                          ENCODER_COUNTS_PER_TURN - u16_start) &
                         ENCODER_ANGLE_MASK);
    if ((0U != u16_span) && (ENCODER_MIN_RANGE > u16_span))
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_wrapper_encoder_iic_status(core_iic_update_word(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_ZPOS,
        ENCODER_ANGLE_MASK, u16_start));
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }
    status = bsp_wrapper_encoder_iic_status(core_iic_update_word(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_MPOS,
        ENCODER_ANGLE_MASK, u16_stop));
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }
    return bsp_wrapper_encoder_iic_status(core_iic_update_word(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_MANG,
        ENCODER_ANGLE_MASK, 0U));
}
/**
 * @brief 仅更新 CONF 的 PM 和 WD 位，保留其他设置。
 * @param[in] mode : 电源模式。
 * @param[in] watchdog_enabled : 低功耗看门狗使能。
 * @return bsp_wrapper_encoder_status_t : 设置结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_set_power_mode(
    bsp_wrapper_encoder_power_mode_t mode, bool watchdog_enabled)
{
    uint16_t u16_value;

    //-------------------------参数检查 begin-------------------------//
    if ((unsigned int)BSP_WRAPPER_ENCODER_POWER_LPM3 < (unsigned int)mode)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    u16_value = (uint16_t)mode;
    if (true == watchdog_enabled)
    {
        u16_value = (uint16_t)(u16_value | ENCODER_CONF_WD_MASK);
    }
    return bsp_wrapper_encoder_iic_status(core_iic_update_word(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_CONF,
        (uint16_t)(ENCODER_CONF_PM_MASK | ENCODER_CONF_WD_MASK), u16_value));
}
/**
 * @brief 设置片内慢滤波强度、快滤波阈值和迟滞，保留 CONF 其他位。
 * @param[in] slow_filter : 慢滤波强度，16x/8x/4x/2x。
 * @param[in] fast_filter : 快滤波阈值，SLOW_ONLY 表示只用慢滤波。
 * @param[in] hysteresis : 输出迟滞，0~3 LSB。
 * @return bsp_wrapper_encoder_status_t : 参数、接口状态或传输结果。
 * @note 只修改易失 CONF 寄存器，掉电恢复默认；
 *       需永久生效须调用 bsp_wrapper_encoder_save_config(BSP_WRAPPER_ENCODER_SAVE_SETTING)。
 *       低功耗模式下慢滤波被芯片强制，SF 设置可能不生效。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_set_filter(
    bsp_wrapper_encoder_slow_filter_t slow_filter,
    bsp_wrapper_encoder_fast_filter_t fast_filter,
    bsp_wrapper_encoder_hysteresis_t hysteresis)
{
    uint16_t u16_value;

    //-------------------------参数检查 begin-------------------------//
    if (((unsigned int)BSP_WRAPPER_ENCODER_SLOW_FILTER_2X <
         (unsigned int)slow_filter) ||
        ((unsigned int)BSP_WRAPPER_ENCODER_FAST_FILTER_10LSB <
         (unsigned int)fast_filter) ||
        ((unsigned int)BSP_WRAPPER_ENCODER_HYSTERESIS_3LSB <
         (unsigned int)hysteresis))
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    u16_value = (uint16_t)(
        ((uint16_t)slow_filter << ENCODER_CONF_SF_SHIFT) |
        ((uint16_t)fast_filter << ENCODER_CONF_FTH_SHIFT) |
        ((uint16_t)hysteresis << ENCODER_CONF_HYST_SHIFT));

    return bsp_wrapper_encoder_iic_status(core_iic_update_word(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_CONF,
        (uint16_t)(ENCODER_CONF_SF_MASK | ENCODER_CONF_FTH_MASK |
                   ENCODER_CONF_HYST_MASK), u16_value));
}
/**
 * @brief 检查条件后执行一次烧录，重载 OTP 并核对目标寄存器。
 * @param[in] target : 角度范围或配置保存区域。
 * @return bsp_wrapper_encoder_status_t : 操作结果；发出烧录后失败不可自动重试。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_save_config(
    bsp_wrapper_encoder_save_t target)
{
    uint8_t u8_zmco = 0U;
    uint8_t u8_magnet_status = 0U;
    uint8_t u8_command;
    uint8_t u8_register;
    uint8_t a_u8_expected[4] = {0U, 0U, 0U, 0U};
    uint8_t a_u8_actual[4] = {0U, 0U, 0U, 0U};
    uint8_t u8_index;
    const uint8_t a_u8_reload[3] = {0x01U, 0x11U, 0x10U};
    bsp_wrapper_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((unsigned int)BSP_WRAPPER_ENCODER_SAVE_SETTING < (unsigned int)target)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_wrapper_encoder_iic_status(core_iic_read(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_ZMCO, &u8_zmco, 1U));
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }
    u8_zmco &= ENCODER_ZMCO_MASK;
    if (BSP_WRAPPER_ENCODER_SAVE_ANGLE == target)
    {
        if (3U <= u8_zmco)
        {
            return BSP_WRAPPER_ENCODER_ERR_STATE;
        }
        status = bsp_wrapper_encoder_iic_status(core_iic_read(
            BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_STATUS,
            &u8_magnet_status, 1U));
        if (BSP_WRAPPER_ENCODER_OK != status)
        {
            return status;
        }
        if (0U == (u8_magnet_status & ENCODER_STATUS_MD_MASK))
        {
            return BSP_WRAPPER_ENCODER_ERR_STATE;
        }
        u8_register = ENCODER_REG_ZPOS;
        u8_command = ENCODER_BURN_ANGLE;
    }
    else
    {
        u8_register = ENCODER_REG_MANG;
        u8_command = ENCODER_BURN_SETTING;
    }

    status = bsp_wrapper_encoder_iic_status(core_iic_read(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, u8_register, a_u8_expected, 4U));
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }
    if ((BSP_WRAPPER_ENCODER_SAVE_SETTING == target) && (0U != u8_zmco) &&
        ((0U != (a_u8_expected[0] & 0x0FU)) ||
         (0U != a_u8_expected[1])))
    {
        return BSP_WRAPPER_ENCODER_ERR_STATE;
    }

    /* 此处开始会消耗 OTP 次数；后续任何失败均直接交由调用者处理。 */
    status = bsp_wrapper_encoder_iic_status(core_iic_write(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_BURN, &u8_command, 1U));
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }
    core_delay_us(1000U);
    for (u8_index = 0U; u8_index < 3U; u8_index++)
    {
        status = bsp_wrapper_encoder_iic_status(core_iic_write(
            BSP_WRAPPER_ENCODER_IIC_ADDRESS, ENCODER_REG_BURN,
            &a_u8_reload[u8_index], 1U));
        if (BSP_WRAPPER_ENCODER_OK != status)
        {
            return status;
        }
    }
    status = bsp_wrapper_encoder_iic_status(core_iic_read(
        BSP_WRAPPER_ENCODER_IIC_ADDRESS, u8_register, a_u8_actual, 4U));
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }

    /* 只核对已定义位，不比较芯片保留位。 */
    a_u8_expected[0] &= 0x0FU;
    a_u8_actual[0] &= 0x0FU;
    if (BSP_WRAPPER_ENCODER_SAVE_ANGLE == target)
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
            return BSP_WRAPPER_ENCODER_ERR_VERIFY;
        }
    }
    return BSP_WRAPPER_ENCODER_OK;
}

/**
 * @brief 更新实测缓存，并外推到本次调用时刻的机械角度，不启动 DMA。
 * @param[in] u32_timeout_us : 反馈超时阈值/us，0 表示禁止使用反馈。
 * @param[out] p_angle_deg : 机械角度，单位度。
 * @param[out] p_angular_velocity : 机械角速度，单位 rad/s。
 * @return bsp_wrapper_encoder_status_t : 有效反馈返回 OK，未采样、超时
 * 或参数错误返回对应状态；反馈无效时输出不变。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_update(
    uint32_t u32_timeout_us,
    float *p_angle_deg, float *p_angular_velocity)
{
    uint16_t u16_angle;
    uint32_t u32_now_us;
    uint32_t u32_age_us;
    float f_velocity;
    int32_t i32_angle_delta;
    uint32_t u32_elapsed_us;
    bsp_wrapper_encoder_status_t status;
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_angle_deg) || (NULL == p_angular_velocity))
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//
    /* 只有新样本才更新实测角度和速度，返回状态表示反馈是否可用。 */
    status = bsp_wrapper_encoder_get_angle_dma(&u16_angle);
    if (BSP_WRAPPER_ENCODER_OK == status)
    {
        g_f_latest_angle_deg =
            (float)u16_angle * BSP_WRAPPER_ENCODER_DEG_PER_COUNT;
        if ((0U != g_u8_velocity_has_previous) &&
            (UINT16_MAX != g_encoder.u16_last_angle_count) &&
            (g_encoder.u32_latest_time_us != g_encoder.u32_last_time_us))
        {
            i32_angle_delta = (int32_t)g_encoder.u16_latest_angle -
                             (int32_t)g_encoder.u16_last_angle_count;
            if (BSP_WRAPPER_ENCODER_HALF_TURN < i32_angle_delta)
            {
                i32_angle_delta -= BSP_WRAPPER_ENCODER_COUNTS_PER_TURN;
            }
            else if (-BSP_WRAPPER_ENCODER_HALF_TURN > i32_angle_delta)
            {
                i32_angle_delta += BSP_WRAPPER_ENCODER_COUNTS_PER_TURN;
            }

            u32_elapsed_us = core_time_get_us_delta(
                g_encoder.u32_latest_time_us,
                g_encoder.u32_last_time_us);
            f_velocity = ((float)i32_angle_delta *
                BSP_WRAPPER_ENCODER_RAD_PER_COUNT *
                BSP_WRAPPER_ENCODER_US_PER_SECOND) / (float)u32_elapsed_us;
            g_f_latest_angle_velocity = f_velocity;
            /* 只累加新样本；1kHz 读取时有几个样本就平均几个。 */
            g_encoder.f_velocity_sum += f_velocity;
            g_encoder.u32_velocity_count++;
        }
        g_u8_velocity_has_previous = 1U;
    }
    if (UINT16_MAX == g_encoder.u16_latest_angle)
    {
        return BSP_WRAPPER_ENCODER_ERR_EMPTY;
    }
    /* 同一次样本年龄同时用于超时检查和角度外推。 */
    u32_now_us = core_time_get_us();
    u32_age_us = core_time_get_us_delta(
        u32_now_us, g_encoder.u32_latest_time_us);
    if ((0U == u32_timeout_us) || (u32_age_us > u32_timeout_us))
    {
        return BSP_WRAPPER_ENCODER_ERR_TIMEOUT;
    }
    *p_angle_deg = g_f_latest_angle_deg;
    /* 用角速度补偿采样时刻到当前时刻之间的角度变化。 */
    *p_angle_deg += g_f_latest_angle_velocity *
        (float)u32_age_us *
        (57.2957795131f / BSP_WRAPPER_ENCODER_US_PER_SECOND);

    //防止角度超过360°,考虑回绕
    if (*p_angle_deg >= 360.0f) { *p_angle_deg -= 360.0f; }
       else if (*p_angle_deg < 0.0f) { *p_angle_deg += 360.0f; }

    *p_angular_velocity = g_f_latest_angle_velocity;//保存最新角速度
    return BSP_WRAPPER_ENCODER_OK;
}

/** @brief 重置反馈时间基准及测速历史，保留最新机械角度。 */
void bsp_wrapper_encoder_reset_feedback_history(void)
{
    g_encoder.u32_latest_time_us = core_time_get_us();
    g_encoder.u32_last_time_us = g_encoder.u32_latest_time_us;
    g_encoder.u8_sample_ready = 0U;
    g_encoder.u16_last_angle_count = UINT16_MAX;
    g_f_latest_angle_velocity = 0.0f;
    g_encoder.f_average_velocity_rad_s = 0.0f;
    g_encoder.f_filtered_velocity_rad_s = 0.0f;
    g_encoder.u32_velocity_filter_time_us = 0U;
    g_encoder.u8_velocity_filter_valid = 0U;
    g_u8_velocity_has_previous = 0U;
    g_encoder.f_velocity_sum = 0.0f;
    g_encoder.u32_velocity_count = 0U;
}

