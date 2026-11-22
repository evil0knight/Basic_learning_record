/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_wrapper_current.h
 *
 * @par dependencies
 * - ../../../platform_mcu/adc/inc/adc_port.h
 *
 * @author YA
 *
 * @brief FOC Injected ADC 电流读取与换算服务接口。
 *
 * Processing flow:
 *
 * ADC Injected 完成 -> 读取 rank -> 偏置/比例换算 -> 返回三相电流。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef BSP_WRAPPER_CURRENT_H
#define BSP_WRAPPER_CURRENT_H

//****************************** Includes begin ******************************//
#include "../../../platform_mcu/adc/inc/adc_port.h"
//******************************* Includes end *******************************//

//******************************* Enums begin ********************************//
/** @brief 电流 Wrapper 操作状态，不依赖应用层状态类型。 */
typedef enum
{
    BSP_WRAPPER_CURRENT_OK = 0,              /* 操作成功。 */
    BSP_WRAPPER_CURRENT_ERROR = -1,          /* 读取或换算失败。 */
    BSP_WRAPPER_CURRENT_ERR_ARGUMENT = -2,   /* 参数错误。 */
    BSP_WRAPPER_CURRENT_ERR_STATE = -3       /* 尚未完成零偏标定。 */
} bsp_wrapper_current_status_t;
//******************************** Enums end *********************************//

//************************ Function Prototypes begin *************************//
/** @brief 初始化电流采样服务。*/
bsp_wrapper_current_status_t bsp_wrapper_current_init(void);

/**
 * @brief 写入三相 ADC 零偏，仅保存标定结果。
 * @note 调用方保证暂停电流控制；不在此采样或检查电机状态。
 */
void bsp_wrapper_current_set_offset(
    uint16_t u16_offset_a,
    uint16_t u16_offset_b,
    uint16_t u16_offset_c);

/** @brief 读取当前 RAM 中保存的三相 ADC 零偏 */
void bsp_wrapper_current_get_offset(
    uint16_t *p_offset_a,
    uint16_t *p_offset_b,
    uint16_t *p_offset_c);

/** @brief 按参考工程公式将单相 ADC 计数转换为电流  */
bsp_wrapper_current_status_t bsp_wrapper_current_convert(
    uint16_t u16_raw,
    uint16_t u16_zero_offset,
    float *p_current);

/** @brief 读取并换算指定 ADC 的三相电流。 */
bsp_wrapper_current_status_t bsp_wrapper_current_read(
    en_core_adc_instance_t instance,
    float *p_phase_a,
    float *p_phase_b,
    float *p_phase_c);
//************************* Function Prototypes end **************************//

#endif /* BSP_WRAPPER_CURRENT_H */
