/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file service_foc.c
 *
 * @par dependencies
 * - service_foc.h
 * - ../../04_platform/platform_bsp/encoder/adapter/bsp_adapter_encoder.h
 * - ../../04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 实现 FOC 服务层的编码器角度采样与缓存。
 *
 * Processing flow:
 *
 * init 挂载编码器 -> 20kHz节拍异步采样 -> 缓存 12 位计数。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "service_foc.h"

#include "../../04_platform/platform_bsp/encoder/adapter/bsp_adapter_encoder.h"
#include "../../04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
/* 角度缓存相关参数。 */
#define SERVICE_FOC_ENCODER_DIV      (4U)          // 20kHz / 4 = 5kHz
#define SERVICE_FOC_COUNTS_PER_TURN  (4096.0f)     // 编码器每圈计数
#define SERVICE_FOC_TWO_PI           (6.28318530718f) // 2π
//******************************* Defines end ********************************//

//************************** Static Variables begin **************************//
/* 最近一次从 ANGLE 寄存器取回的 12 位计数值。 */
static volatile uint16_t g_u16_latest_angle_count = 0U;
static uint8_t           g_u8_encoder_tick = 0U;  // 20kHz 分频计数
static uint8_t           g_u8_initialized = 0U;   // 服务是否已初始化
//*************************** Static Variables end ***************************//

//************************** Static Functions begin **************************//
/**
 * @brief 把 Wrapper 状态转换为应用层状态。
 *
 * @param[in] status : Wrapper 返回状态。
 *
 * @return app_status_t : 对应的应用层状态。
 */
static app_status_t service_foc_status_convert(
    bsp_wrapper_encoder_status_t status)
{
    switch (status)
    {
        case BSP_WRAPPER_ENCODER_OK:
            return APP_STATUS_OK;

        case BSP_WRAPPER_ENCODER_ERR_ARGUMENT:
            return APP_STATUS_INVALID_PARAM;

        default:
            return APP_STATUS_IO_ERROR;
    }
}
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 初始化 FOC 服务并挂载编码器。
 *
 * @return app_status_t : 成功或 Wrapper 返回状态。
 */
app_status_t service_foc_init(void)
{
    app_status_t status;

    //-------------------------参数检查 begin-------------------------//
    /* 无输入参数。 */
    //--------------------------参数检查 end--------------------------//

    status = service_foc_status_convert(
        bsp_wrapper_encoder_init((uint32_t)BSP_ENCODER_ID_MOTOR));
    if (APP_STATUS_OK != status)
    {
        return status;
    }

    g_u16_latest_angle_count = 0U;
    g_u8_encoder_tick = 0U;
    g_u8_initialized = 1U;

    /* 先启动第一笔，后续由 HRTIM 20kHz 中断每 4 次调用一次。 */
    return service_foc_status_convert(
        bsp_wrapper_encoder_start_dma((uint32_t)BSP_ENCODER_ID_MOTOR));
}

/**
 * @brief 20kHz 控制节拍中的编码器采样与消费。
 *
 * @return void : 无返回值。
 * @note 每 4 个 tick 启动一笔 DMA，每个 tick 消费 AB 缓冲中的新样本。
 */
void service_foc_encoder_tick(void)
{
    uint16_t u16_angle_count;
    bsp_wrapper_encoder_status_t status;

    if (0U == g_u8_initialized)
    {
        return;
    }

    g_u8_encoder_tick++;
    if (SERVICE_FOC_ENCODER_DIV <= g_u8_encoder_tick)
    {
        g_u8_encoder_tick = 0U;
        (void)bsp_wrapper_encoder_start_dma(
            (uint32_t)BSP_ENCODER_ID_MOTOR);
    }

    status = bsp_wrapper_encoder_get_dma_angle(
        (uint32_t)BSP_ENCODER_ID_MOTOR, &u16_angle_count);
    if (BSP_WRAPPER_ENCODER_OK == status)
    {
        g_u16_latest_angle_count = u16_angle_count;
    }
}

/**
 * @brief 主循环轮询 FOC 服务，当前只维护初始化状态。
 *
 * @param[in] u32_now_ms : 当前毫秒时间。
 *
 * @return app_status_t : 成功或初始化状态。
 */
app_status_t service_foc_poll(uint32_t u32_now_ms)
{
    (void)u32_now_ms;

    if (0U == g_u8_initialized)
    {
        return service_foc_init();
    }

    return APP_STATUS_OK;
}

/**
 * @brief 获取最近一次缓存的 12 位角度计数。
 *
 * @param[out] p_u16_angle : 12 位角度计数。
 *
 * @return app_status_t : 成功或参数错误。
 */
app_status_t service_foc_get_angle_count(uint16_t *p_u16_angle)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_u16_angle)
    {
        return APP_STATUS_INVALID_PARAM;
    }
    //--------------------------参数检查 end--------------------------//

    *p_u16_angle = g_u16_latest_angle_count;
    return APP_STATUS_OK;
}

/**
 * @brief 把 12 位角度计数转换为机械角度。
 *
 * @param[in] u16_count : 12 位角度计数。
 *
 * @return float : 机械角度，单位 rad。
 */
float service_foc_count_to_mech_rad(uint16_t u16_count)
{
    return ((float)u16_count * SERVICE_FOC_TWO_PI) /
           SERVICE_FOC_COUNTS_PER_TURN;
}
//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
