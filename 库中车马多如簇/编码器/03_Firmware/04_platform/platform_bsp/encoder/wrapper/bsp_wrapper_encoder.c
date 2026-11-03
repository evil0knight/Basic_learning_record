/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_wrapper_encoder.c
 *
 * @par dependencies
 * - bsp_wrapper_encoder.h
 * - stdbool.h
 * - stddef.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 按索引保存编码器器件函数表，并把每个操作直接转发到对应函数。
 *
 * Processing flow:
 *
 * reg 写索引并保存函数表 -> 对外接口按索引取器件 -> 调用对应操作函数。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "bsp_wrapper_encoder.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//************************** Static Variables begin **************************//
/* 器件函数表副本，一个 Adapter 占一个索引。 */
static bsp_wrapper_encoder_dev_t
    g_a_bsp_wrapper_encoder_dev[BSP_WRAPPER_ENCODER_DRV_MAX_NUM];

/* 每个索引是否已注册。 */
static bool
    g_a_bsp_wrapper_encoder_registered[BSP_WRAPPER_ENCODER_DRV_MAX_NUM];
//*************************** Static Variables end ***************************//

//************************** Static Functions begin **************************//
/**
 * @brief 按索引取已注册的器件函数表。
 *
 * Steps:
 *  1. 索引越界返回 ERR_ARGUMENT。
 *  2. 索引未注册返回 ERR_STATE。
 *  3. 有效时把器件地址写回调用者。
 *
 * @param[in] u32_index : 器件索引。
 * @param[out] pp_dev : 器件函数表地址的输出指针。
 *
 * @return bsp_wrapper_encoder_status_t : 成功返回 OK，否则为具体错误。
 *
 */
static bsp_wrapper_encoder_status_t bsp_wrapper_encoder_get(
    uint32_t u32_index,
    bsp_wrapper_encoder_dev_t **pp_dev)
{
    //-------------------------参数检查 begin-------------------------//
    if (BSP_WRAPPER_ENCODER_DRV_MAX_NUM <= u32_index)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    if (!g_a_bsp_wrapper_encoder_registered[u32_index])
    {
        return BSP_WRAPPER_ENCODER_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    *pp_dev = &g_a_bsp_wrapper_encoder_dev[u32_index];
    return BSP_WRAPPER_ENCODER_OK;
}
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 把编码器器件函数表注册到指定索引。
 *
 * Steps:
 *  1. 检查索引、器件指针及 7 个操作函数是否有效。
 *  2. 检查该索引是否已被占用，未占用时整体保存并置位标志。
 *  3. 把本次索引写回器件的 u32_idx 字段。
 *
 * @param[in] u32_index : 器件索引。
 * @param[in] p_dev : 器件函数表。
 *
 * @return bool : 注册成功返回 true；参数无效或索引被占返回 false。
 *
 */
bool bsp_wrapper_encoder_reg(uint32_t u32_index,
                             bsp_wrapper_encoder_dev_t *p_dev)
{
    //-------------------------参数检查 begin-------------------------//
    if ((BSP_WRAPPER_ENCODER_DRV_MAX_NUM <= u32_index) ||
        (NULL == p_dev) || (NULL == p_dev->init) ||
        (NULL == p_dev->read_angle) || (NULL == p_dev->read_raw_angle) ||
        (NULL == p_dev->set_range) || (NULL == p_dev->set_power_mode) ||
        (NULL == p_dev->set_filter) || (NULL == p_dev->save_config) ||
        (NULL == p_dev->start_dma) || (NULL == p_dev->stop_dma) ||
        (NULL == p_dev->get_dma_angle))
    {
        return false;
    }
    if (g_a_bsp_wrapper_encoder_registered[u32_index])
    {
        return false;
    }
    //--------------------------参数检查 end--------------------------//

    g_a_bsp_wrapper_encoder_dev[u32_index]         = *p_dev;
    g_a_bsp_wrapper_encoder_dev[u32_index].u32_idx = u32_index;
    g_a_bsp_wrapper_encoder_registered[u32_index] = true;
    return true;
}

/**
 * @brief 初始化指定索引的编码器器件。
 *
 * Steps:
 *  1. 检查索引是否已注册。
 *  2. 调用该器件的 init。
 *
 * @param[in] u32_index : 器件索引。
 *
 * @return bsp_wrapper_encoder_status_t : 初始化状态。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_init(uint32_t u32_index)
{
    bsp_wrapper_encoder_dev_t *p_dev;
    bsp_wrapper_encoder_status_t status;

    status = bsp_wrapper_encoder_get(u32_index, &p_dev);
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }

    return p_dev->init(p_dev);
}

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
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_read_angle(
    uint32_t u32_index,
    uint16_t *p_u16_angle)
{
    bsp_wrapper_encoder_dev_t *p_dev;
    bsp_wrapper_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_u16_angle)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_wrapper_encoder_get(u32_index, &p_dev);
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }

    return p_dev->read_angle(p_dev, p_u16_angle);
}

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
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_read_raw_angle(
    uint32_t u32_index,
    uint16_t *p_u16_raw_angle)
{
    bsp_wrapper_encoder_dev_t *p_dev;
    bsp_wrapper_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_u16_raw_angle)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_wrapper_encoder_get(u32_index, &p_dev);
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }

    return p_dev->read_raw_angle(p_dev, p_u16_raw_angle);
}

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
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_set_range(
    uint32_t u32_index,
    const bsp_encoder_range_t *p_range)
{
    bsp_wrapper_encoder_dev_t *p_dev;
    bsp_wrapper_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_range)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_wrapper_encoder_get(u32_index, &p_dev);
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }

    return p_dev->set_range(p_dev, p_range);
}

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
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_set_power_mode(
    uint32_t u32_index,
    const bsp_encoder_power_config_t *p_config)
{
    bsp_wrapper_encoder_dev_t *p_dev;
    bsp_wrapper_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_config)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_wrapper_encoder_get(u32_index, &p_dev);
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }

    return p_dev->set_power_mode(p_dev, p_config);
}

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
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 * @note 只写易失 CONF，掉电恢复默认；永久生效需烧 SETTING。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_set_filter(
    uint32_t u32_index,
    const bsp_encoder_filter_t *p_filter)
{
    bsp_wrapper_encoder_dev_t *p_dev;
    bsp_wrapper_encoder_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_filter)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    status = bsp_wrapper_encoder_get(u32_index, &p_dev);
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }

    return p_dev->set_filter(p_dev, p_filter);
}

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
 * @return bsp_wrapper_encoder_status_t : 操作状态。
 * @note 烧录有次数限制且不可自动重试，调用前须确认供电与烧录条件。
 *
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_save_config(
    uint32_t u32_index,
    bsp_encoder_save_t target)
{
    bsp_wrapper_encoder_dev_t *p_dev;
    bsp_wrapper_encoder_status_t status;

    status = bsp_wrapper_encoder_get(u32_index, &p_dev);
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }

    return p_dev->save_config(p_dev, target);
}

/**
 * @brief 启动指定编码器的异步角度 DMA 采样。
 *
 * @param[in] u32_index : 器件索引。
 *
 * @return bsp_wrapper_encoder_status_t : 启动结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_start_dma(
    uint32_t u32_index)
{
    bsp_wrapper_encoder_dev_t *p_dev;
    bsp_wrapper_encoder_status_t status;

    status = bsp_wrapper_encoder_get(u32_index, &p_dev);
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }

    return p_dev->start_dma(p_dev);
}

/**
 * @brief 停止指定编码器的异步角度 DMA 采样。
 *
 * @param[in] u32_index : 器件索引。
 *
 * @return bsp_wrapper_encoder_status_t : 停止结果。
 */
bsp_wrapper_encoder_status_t bsp_wrapper_encoder_stop_dma(
    uint32_t u32_index)
{
    bsp_wrapper_encoder_dev_t *p_dev;
    bsp_wrapper_encoder_status_t status;

    status = bsp_wrapper_encoder_get(u32_index, &p_dev);
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }

    return p_dev->stop_dma(p_dev);
}

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
    uint16_t *p_u16_angle)
{
    bsp_wrapper_encoder_dev_t *p_dev;
    bsp_wrapper_encoder_status_t status;

    if (NULL == p_u16_angle)
    {
        return BSP_WRAPPER_ENCODER_ERR_ARGUMENT;
    }

    status = bsp_wrapper_encoder_get(u32_index, &p_dev);
    if (BSP_WRAPPER_ENCODER_OK != status)
    {
        return status;
    }

    return p_dev->get_dma_angle(p_dev, p_u16_angle);
}

//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
