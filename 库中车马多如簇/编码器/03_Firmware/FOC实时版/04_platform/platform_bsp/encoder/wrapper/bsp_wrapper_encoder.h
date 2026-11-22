/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_wrapper_encoder.h
 *
 * @par dependencies
 *
 * @author YA
 *
 * @brief 声明不依赖 HAL 类型的编码器 Wrapper 接口。
 *
 * Processing flow:
 *
 * Service 调用 Wrapper；I2C 寄存器访问由 Platform core_iic 完成。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

#ifndef BSP_WRAPPER_ENCODER_H
#define BSP_WRAPPER_ENCODER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BSP_WRAPPER_ENCODER_OK = 0,
    BSP_WRAPPER_ENCODER_ERROR = -1,
    BSP_WRAPPER_ENCODER_ERR_ARGUMENT = -2,
    BSP_WRAPPER_ENCODER_ERR_STATE = -3,
    BSP_WRAPPER_ENCODER_ERR_VERIFY = -4,
    BSP_WRAPPER_ENCODER_ERR_TIMEOUT = -5,
    BSP_WRAPPER_ENCODER_ERR_BUSY = -6,
    BSP_WRAPPER_ENCODER_ERR_NACK = -7,
    BSP_WRAPPER_ENCODER_ERR_EMPTY = -8
} bsp_wrapper_encoder_status_t;

typedef enum
{
    BSP_WRAPPER_ENCODER_POWER_NORMAL = 0,
    BSP_WRAPPER_ENCODER_POWER_LPM1,
    BSP_WRAPPER_ENCODER_POWER_LPM2,
    BSP_WRAPPER_ENCODER_POWER_LPM3
} bsp_wrapper_encoder_power_mode_t;

typedef enum
{
    BSP_WRAPPER_ENCODER_SAVE_ANGLE = 0,
    BSP_WRAPPER_ENCODER_SAVE_SETTING
} bsp_wrapper_encoder_save_t;

typedef enum
{
    BSP_WRAPPER_ENCODER_SLOW_FILTER_16X = 0,    /* 16x，阶跃 2.2ms，噪声最小 */
    BSP_WRAPPER_ENCODER_SLOW_FILTER_8X,         /* 8x，阶跃 1.1ms */
    BSP_WRAPPER_ENCODER_SLOW_FILTER_4X,         /* 4x，阶跃 0.55ms */
    BSP_WRAPPER_ENCODER_SLOW_FILTER_2X          /* 2x，阶跃 0.286ms，噪声最大 */
} bsp_wrapper_encoder_slow_filter_t;

typedef enum
{
    BSP_WRAPPER_ENCODER_FAST_FILTER_SLOW_ONLY = 0,  /* 000：只用慢滤波 */
    BSP_WRAPPER_ENCODER_FAST_FILTER_6LSB,           /* 001：阈值 6 LSB */
    BSP_WRAPPER_ENCODER_FAST_FILTER_7LSB,           /* 010：阈值 7 LSB */
    BSP_WRAPPER_ENCODER_FAST_FILTER_9LSB,           /* 011：阈值 9 LSB */
    BSP_WRAPPER_ENCODER_FAST_FILTER_18LSB,          /* 100：阈值 18 LSB */
    BSP_WRAPPER_ENCODER_FAST_FILTER_21LSB,          /* 101：阈值 21 LSB */
    BSP_WRAPPER_ENCODER_FAST_FILTER_24LSB,          /* 110：阈值 24 LSB */
    BSP_WRAPPER_ENCODER_FAST_FILTER_10LSB           /* 111：阈值 10 LSB */
} bsp_wrapper_encoder_fast_filter_t;

typedef enum
{
    BSP_WRAPPER_ENCODER_HYSTERESIS_OFF = 0,     /* 00：关闭迟滞 */
    BSP_WRAPPER_ENCODER_HYSTERESIS_1LSB,        /* 01：1 LSB */
    BSP_WRAPPER_ENCODER_HYSTERESIS_2LSB,        /* 10：2 LSB */
    BSP_WRAPPER_ENCODER_HYSTERESIS_3LSB         /* 11：3 LSB */
} bsp_wrapper_encoder_hysteresis_t;

/* 所有指针由调用者保证有效；配置和 OTP 操作须停止采样后串行调用。 */
/** @brief 初始化软件缓存；不修改外设和编码器寄存器。 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_init(void);
/**
 * @brief 消费已完成样本，检查反馈超时并外推角度，不启动 DMA。
 * @param[in] u32_timeout_us : 反馈超时阈值/us，0 表示禁止使用反馈。
 * @param[out] p_angle_deg : 机械角度，[0, 360)，单位度。
 * @param[out] p_angular_velocity : 机械角速度，单位 rad/s。
 * @return bsp_wrapper_encoder_status_t : 反馈有效返回 OK，未采样返回
 * EMPTY，反馈过期返回 TIMEOUT，指针无效返回 ARGUMENT。
 * @note 由上层安排调用节拍；仅在 ADC1_2 中断调用。
 * 无新样本但缓存未过期时仍返回 OK，速度保持缓存、角度继续外推。
 * 反馈无效时不写入输出；新样本仍会被消费并更新内部缓存。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_update(
    uint32_t u32_timeout_us,
    float *p_angle_deg, float *p_angular_velocity);
/** @brief 重置反馈时间基准及测速累计历史，保留最新机械角度。 */
void bsp_wrapper_encoder_reset_feedback_history(void);
/**
 * @brief 堵塞读取处理后角度，发布新反馈，DMA 在途时返回忙。
 * @param[out] p_u16_angle : 处理后的角度计数。
 * @return bsp_wrapper_encoder_status_t : 读取结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_read_angle(
    uint16_t *p_u16_angle);

/**
 * @brief 读取原始角度的 12 位计数。
 * @param[out] p_u16_angle : 原始角度计数。
 * @return bsp_wrapper_encoder_status_t : 读取结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_read_raw_angle(
    uint16_t *p_u16_angle);

/**
 * @brief 启动一次角度 DMA 接收，由外部采样节拍反复调用。
 *
 *
 * @return bsp_wrapper_encoder_status_t : 启动结果；上一笔在飞时返回 BUSY。
 * @note 本函数不再由完成回调自动续传，固定采样由 HRTIM 分频触发。
 */
/* 并发约束：仅在 ADC1_2 采样中断调用；ADC1_2、I2C1 EV/ER 和
 * I2C1 DMA 必须保持相同抢占优先级，不允许主循环并发启动。 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_start_angle_dma(
    void);

/**
 * @brief 停止角度连续 DMA 接收。
 *
 *
 * @return bsp_wrapper_encoder_status_t : 停止结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_stop_angle_dma(
    void);

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
/* 仅在 ADC1_2 中断消费；发布回调必须保持相同抢占优先级。 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_get_angle_dma(
    uint16_t *p_u16_angle);

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
 * @note 空窗口不修改输出；仅由同一控制中断的 1kHz 调度读取。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_get_angular_velocity(
    float *p_f_velocity);

/**
 * @brief 设置起止位置并清零 MANG，支持跨越角度零点的范围。
 * @param[in] u16_start : 起始原始角度计数。
 * @param[in] u16_stop : 终止原始角度计数。
 * @return bsp_wrapper_encoder_status_t : 设置结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_set_range(
    uint16_t u16_start, uint16_t u16_stop);

/**
 * @brief 仅更新 CONF 的 PM 和 WD 位，保留其他设置。
 * @param[in] mode : 电源模式。
 * @param[in] watchdog_enabled : 低功耗看门狗使能。
 * @return bsp_wrapper_encoder_status_t : 设置结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_set_power_mode(
    bsp_wrapper_encoder_power_mode_t mode, bool watchdog_enabled);

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
    bsp_wrapper_encoder_hysteresis_t hysteresis);

/**
 * @brief 检查条件后执行一次烧录，重载 OTP 并核对目标寄存器。
 * @param[in] target : 角度范围或配置保存区域。
 * @return bsp_wrapper_encoder_status_t : 操作结果；发出烧录后失败不可自动重试。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_save_config(
    bsp_wrapper_encoder_save_t target);

/** @brief DMA 成功时发布角度，失败时保留已发布结果。 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_dma_done(bool success);
#endif
