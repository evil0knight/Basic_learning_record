/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_wrapper_encoder.h
 *
 * @par dependencies
 * - ../../../../05_bsp/bsp_encoder/handler/bsp_encoder_handler.h
 * - stdbool.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 声明编码器 BSP Wrapper 的状态码、器件函数表和对外接口。
 *
 * Processing flow:
 *
 * Adapter reg(索引, dev) -> 按索引取器件 -> 调用对应操作函数。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef BSP_WRAPPER_ENCODER_H
#define BSP_WRAPPER_ENCODER_H

//****************************** Includes begin ******************************//
#include "../../../../05_bsp/bsp_encoder/handler/bsp_encoder_handler.h"

#include <stdbool.h>
#include <stdint.h>
//******************************* Includes end *******************************//

//******************************* Defines begin ******************************//
/* 允许同时注册的编码器数量；reg 时的索引上限。 */
#define BSP_WRAPPER_ENCODER_DRV_MAX_NUM    (2U)
//******************************** Defines end *******************************//

//******************************* Enums begin ********************************//
/* Wrapper 对外统一状态码，0 表示成功，负数表示失败。 */
typedef enum
{
    BSP_WRAPPER_ENCODER_OK = 0,
    BSP_WRAPPER_ENCODER_ERROR = -1,
    BSP_WRAPPER_ENCODER_ERR_ARGUMENT = -2,
    BSP_WRAPPER_ENCODER_ERR_STATE = -3,
    BSP_WRAPPER_ENCODER_ERR_TIMEOUT = -4,
    BSP_WRAPPER_ENCODER_ERR_BUSY = -5,
    BSP_WRAPPER_ENCODER_ERR_NACK = -6
} bsp_wrapper_encoder_status_t;
//******************************** Enums end *********************************//

//****************************** Typedefs begin ******************************//
/**
 * @brief 编码器器件函数表：一个 Adapter 填一份注册进来。
 * @note u32_idx 由 Wrapper 在 reg 时写入，Adapter 用它定位自己的器件资源；
 *       p_user_data 由 Adapter 自行维护，Wrapper 只做原样透传。
 */
typedef struct bsp_wrapper_encoder_dev
{
    uint32_t u32_idx;                       /* 器件索引，reg 时写入。 */
    void    *p_user_data;                   /* Adapter 自管上下文。 */

    /* 挂载 Handler/Driver，只保存资源，不访问芯片。 */
    bsp_wrapper_encoder_status_t (*init)(
        struct bsp_wrapper_encoder_dev *p_dev);

    /* 读取经过范围缩放和滤波的 ANGLE 值。 */
    bsp_wrapper_encoder_status_t (*read_angle)(
        struct bsp_wrapper_encoder_dev *p_dev,
        uint16_t *p_u16_angle);

    /* 读取未经范围缩放的 RAW ANGLE 值。 */
    bsp_wrapper_encoder_status_t (*read_raw_angle)(
        struct bsp_wrapper_encoder_dev *p_dev,
        uint16_t *p_u16_raw_angle);

    /* 设置 ZPOS/MPOS 测量起止范围。 */
    bsp_wrapper_encoder_status_t (*set_range)(
        struct bsp_wrapper_encoder_dev *p_dev,
        const bsp_encoder_range_t *p_range);

    /* 设置电源模式和低功耗看门狗开关。 */
    bsp_wrapper_encoder_status_t (*set_power_mode)(
        struct bsp_wrapper_encoder_dev *p_dev,
        const bsp_encoder_power_config_t *p_config);

    /* 设置慢滤波强度、快滤波阈值和迟滞。 */
    bsp_wrapper_encoder_status_t (*set_filter)(
        struct bsp_wrapper_encoder_dev *p_dev,
        const bsp_encoder_filter_t *p_filter);

    /* 烧录选定的 OTP 区域。 */
    bsp_wrapper_encoder_status_t (*save_config)(
        struct bsp_wrapper_encoder_dev *p_dev,
        bsp_encoder_save_t target);

    /* 启动一笔异步角度 DMA 采样。 */
    bsp_wrapper_encoder_status_t (*start_dma)(
        struct bsp_wrapper_encoder_dev *p_dev);

    /* 停止异步角度 DMA 采样。 */
    bsp_wrapper_encoder_status_t (*stop_dma)(
        struct bsp_wrapper_encoder_dev *p_dev);

    /* 从 AB 双缓冲取得最新角度计数。 */
    bsp_wrapper_encoder_status_t (*get_dma_angle)(
        struct bsp_wrapper_encoder_dev *p_dev,
        uint16_t *p_u16_angle);
} bsp_wrapper_encoder_dev_t;
//******************************* Typedefs end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 把一个编码器器件函数表注册到指定索引。
 *
 * Steps:
 *  1. 检查索引、器件指针及 7 个操作函数是否有效。
 *  2. 检查该索引是否已被占用，未占用时整体保存并置位标志。
 *  3. 把本次索引写回器件的 u32_idx 字段。
 *
 * @param[in] u32_index : 器件索引，范围 0 ~ BSP_WRAPPER_ENCODER_DRV_MAX_NUM-1。
 * @param[in] p_dev     : 器件函数表 {u32_idx, p_user_data, 操作函数...}。
 *
 * @return bool : 注册成功返回 true；参数无效或索引被占返回 false。
 *
 */
bool bsp_wrapper_encoder_reg(uint32_t u32_index,
                             bsp_wrapper_encoder_dev_t *p_dev);

/**
 * @brief 初始化指定索引的编码器器件。
 *
 * Steps:
 *  1. 检查索引是否已注册。
 *  2. 调用该器件的 init，由 Adapter 挂载 Handler/Driver 资源。
 *
 * @param[in] u32_index : 器件索引。
 *
 * @return bsp_wrapper_encoder_status_t : 底层初始化状态；索引无效或未注册
 *         返回 BSP_WRAPPER_ENCODER_ERR_ARGUMENT / ERR_STATE。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_init(uint32_t u32_index);

/**
 * @brief 读取指定编码器经过范围缩放和滤波的 ANGLE 值。
 *
 * Steps:
 *  1. 检查索引和输出指针是否有效。
 *  2. 调用该器件的 read_angle。
 *
 * @param[in] u32_index : 器件索引。
 * @param[out] p_u16_angle : 12 位角度计数，范围 0~4095。
 *
 * @return bsp_wrapper_encoder_status_t : 参数、索引状态或底层结果。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_read_angle(
    uint32_t u32_index,
    uint16_t *p_u16_angle);

/**
 * @brief 读取指定编码器未经范围缩放的 RAW ANGLE 值。
 *
 * Steps:
 *  1. 检查索引和输出指针是否有效。
 *  2. 调用该器件的 read_raw_angle。
 *
 * @param[in] u32_index : 器件索引。
 * @param[out] p_u16_raw_angle : 0~4095 的原始角度计数。
 *
 * @return bsp_wrapper_encoder_status_t : 参数、索引状态或底层结果。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_read_raw_angle(
    uint32_t u32_index,
    uint16_t *p_u16_raw_angle);

/**
 * @brief 设置指定编码器的 ZPOS/MPOS 测量起止范围。
 *
 * Steps:
 *  1. 检查索引和范围指针是否有效。
 *  2. 调用该器件的 set_range。
 *
 * @param[in] u32_index : 器件索引。
 * @param[in] p_range : 起止原始角度计数，各为 0~4095。
 *
 * @return bsp_wrapper_encoder_status_t : 参数、索引状态或底层结果。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_set_range(
    uint32_t u32_index,
    const bsp_encoder_range_t *p_range);

/**
 * @brief 设置指定编码器的电源模式和低功耗看门狗开关。
 *
 * Steps:
 *  1. 检查索引和配置指针是否有效。
 *  2. 调用该器件的 set_power_mode。
 *
 * @param[in] u32_index : 器件索引。
 * @param[in] p_config : 电源模式与看门狗使能配置。
 *
 * @return bsp_wrapper_encoder_status_t : 参数、索引状态或底层结果。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_set_power_mode(
    uint32_t u32_index,
    const bsp_encoder_power_config_t *p_config);

/**
 * @brief 设置指定编码器的慢滤波强度、快滤波阈值和迟滞。
 *
 * Steps:
 *  1. 检查索引和配置指针是否有效。
 *  2. 调用该器件的 set_filter。
 *
 * @param[in] u32_index : 器件索引。
 * @param[in] p_filter : 慢滤波、快滤波阈值与迟滞配置。
 *
 * @return bsp_wrapper_encoder_status_t : 参数、索引状态或底层结果。
 * @note 只写易失 CONF，掉电恢复默认；永久生效需烧 SETTING。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_set_filter(
    uint32_t u32_index,
    const bsp_encoder_filter_t *p_filter);

/**
 * @brief 烧录指定编码器的选定 OTP 区域。
 *
 * Steps:
 *  1. 检查索引是否已注册。
 *  2. 调用该器件的 save_config。
 *
 * @param[in] u32_index : 器件索引。
 * @param[in] target : ANGLE 保存 ZPOS/MPOS，SETTING 保存 MANG/CONF。
 *
 * @return bsp_wrapper_encoder_status_t : 参数、索引状态或底层结果。
 * @note 烧录有次数限制且不可自动重试，调用前须确认供电与烧录条件。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_save_config(
    uint32_t u32_index,
    bsp_encoder_save_t target);

/**
 * @brief 启动指定编码器的异步角度 DMA 采样。
 *
 * @param[in] u32_index : 器件索引。
 *
 * @return bsp_wrapper_encoder_status_t : 启动结果。
 * @note 由固定采样节拍反复调用；上一笔在飞时返回 BUSY。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_start_dma(
    uint32_t u32_index);

/**
 * @brief 停止指定编码器的异步角度 DMA 采样。
 *
 * @param[in] u32_index : 器件索引。
 *
 * @return bsp_wrapper_encoder_status_t : 停止结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_stop_dma(
    uint32_t u32_index);

/**
 * @brief 从 AB 双缓冲取得指定编码器的最新角度计数。
 *
 * @param[in] u32_index : 器件索引。
 * @param[out] p_u16_angle : 12 位角度计数。
 *
 * @return bsp_wrapper_encoder_status_t : 成功、空或底层状态。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_get_dma_angle(
    uint32_t u32_index,
    uint16_t *p_u16_angle);
//************************* Function Prototypes end **************************//

#endif /* BSP_WRAPPER_ENCODER_H */

//******************************* End of File ********************************//
