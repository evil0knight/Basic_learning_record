/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_wrapper_current.c
 *
 * @par dependencies
 * - bsp_wrapper_current.h
 *
 * @author YA
 *
 * @brief FOC Injected ADC 电流读取与换算服务实现。
 *
 * Processing flow:
 *
 * 读取 ADC rank -> 偏置校准 -> 比例换算 -> 输出三相电流。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "bsp_wrapper_current.h"
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
/* ADC 参考电压 2.5V，12 位分辨率：2.5 / 4095，单位 V/count。 */
#define BSP_WRAPPER_CURRENT_ADC_VOLT_PER_COUNT (6.1050061e-4f)
/* SGM42560 检测比率 4000，RREF 为 2kΩ：4000 / 2000，单位 A/V。 */
#define BSP_WRAPPER_CURRENT_AMP_PER_VOLT       (2.0f)
//******************************* Defines end ********************************//

//************************** Static Variables begin **************************//
typedef struct
{
    uint16_t u16_offset_a; /* A 相 ADC 零偏计数。 */
    uint16_t u16_offset_b; /* B 相 ADC 零偏计数。 */
    uint16_t u16_offset_c; /* C 相 ADC 零偏计数。 */
} st_bsp_wrapper_current_t;

static st_bsp_wrapper_current_t g_st_current;
//*************************** Static Variables end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 初始化电流采样服务。
 *
 * Steps:
 *  1. 清空三相 ADC 零偏。
 *
 * @return bsp_wrapper_current_status_t : 初始化结果。
 */
bsp_wrapper_current_status_t bsp_wrapper_current_init(void)
{
    g_st_current.u16_offset_a = 0U;
    g_st_current.u16_offset_b = 0U;
    g_st_current.u16_offset_c = 0U;
    return BSP_WRAPPER_CURRENT_OK;
}

/**
 * @brief 写入标定模块计算的三相 ADC 零偏。
 * @param[in] u16_offset_a : A 相零偏计数。
 * @param[in] u16_offset_b : B 相零偏计数。
 * @param[in] u16_offset_c : C 相零偏计数。
 * @return void : 无返回值。
 * @note 调用方负责暂停电流控制，保证三相结果完整写入。
 */
void bsp_wrapper_current_set_offset(
    uint16_t u16_offset_a,
    uint16_t u16_offset_b,
    uint16_t u16_offset_c)
{
    g_st_current.u16_offset_a = u16_offset_a;
    g_st_current.u16_offset_b = u16_offset_b;
    g_st_current.u16_offset_c = u16_offset_c;
}

void bsp_wrapper_current_get_offset(
    uint16_t *p_offset_a,
    uint16_t *p_offset_b,
    uint16_t *p_offset_c)
{
    if ((NULL != p_offset_a) && (NULL != p_offset_b) &&
        (NULL != p_offset_c))
    {
        *p_offset_a = g_st_current.u16_offset_a;
        *p_offset_b = g_st_current.u16_offset_b;
        *p_offset_c = g_st_current.u16_offset_c;
    }
}

/**
 * @brief 按硬件比例及采样极性将单相 ADC 计数转换为电流。
 *
 * Steps:
 *  1. 检查 12 位 ADC 原始值、零偏范围。
 *  2. 零偏减去原始值以统一极性，再按固定硬件比例换算为电流。
 *
 * @param[in] u16_raw : 12 位 ADC 原始计数，范围 0~4095。
 * @param[in] u16_zero_offset : 本相无电流时的 ADC 零偏计数。
 * @param[out] p_current : 有符号相电流输出地址，单位 A。
 * @return bsp_wrapper_current_status_t : 成功返回 BSP_WRAPPER_CURRENT_OK，否则返回参数错误。
 * @note 极性诊断：暂按 ADC 高于零偏为正，使用开环标定验证。
 */
bsp_wrapper_current_status_t bsp_wrapper_current_convert(
    uint16_t u16_raw,
    uint16_t u16_zero_offset,
    float *p_current)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_current) || (4095U < u16_raw) ||
        (4095U < u16_zero_offset))
    {
        return BSP_WRAPPER_CURRENT_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    *p_current = (((float)u16_raw - (float)u16_zero_offset) *
                 BSP_WRAPPER_CURRENT_ADC_VOLT_PER_COUNT) *
                 BSP_WRAPPER_CURRENT_AMP_PER_VOLT;

    return BSP_WRAPPER_CURRENT_OK;
}

/**
 * @brief 读取并换算指定 ADC 的三相电流。
 *
 * Steps:
 *  1. 检查 ADC 实例；输出指针由调用者保证有效。
 *  2. 读取 Injected rank 1/2/3。
 *  3. 调用转换函数，扣除各相标定零偏并输出三相电流。
 *
 * @param[in] instance : ADC 逻辑实例。
 * @param[out] p_phase_a : A 相电流输出地址，单位 A。
 * @param[out] p_phase_b : B 相电流输出地址，单位 A。
 * @param[out] p_phase_c : C 相电流输出地址，单位 A。
 *
 * @return bsp_wrapper_current_status_t : 读取和换算结果。
 */
bsp_wrapper_current_status_t bsp_wrapper_current_read(
    en_core_adc_instance_t instance,
    float *p_phase_a,
    float *p_phase_b,
    float *p_phase_c)
{
    uint16_t u16_phase_a;
    uint16_t u16_phase_b;
    uint16_t u16_phase_c;

    //-------------------------参数检查 begin-------------------------//
    if ((CORE_ADC2 != instance) || (NULL == p_phase_a) ||
        (NULL == p_phase_b) || (NULL == p_phase_c))
    {
        return BSP_WRAPPER_CURRENT_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    if ((core_adc_get_injected_value(
             instance, ADC_INJECTED_RANK_1, &u16_phase_a) != CORE_ADC_OK) ||
        (core_adc_get_injected_value(
             instance, ADC_INJECTED_RANK_2, &u16_phase_b) != CORE_ADC_OK) ||
        (core_adc_get_injected_value(
             instance, ADC_INJECTED_RANK_3, &u16_phase_c) != CORE_ADC_OK))
    {
        return BSP_WRAPPER_CURRENT_ERROR;
    }

    if ((BSP_WRAPPER_CURRENT_OK != bsp_wrapper_current_convert(
             u16_phase_a, g_st_current.u16_offset_a, p_phase_a)) ||
        (BSP_WRAPPER_CURRENT_OK != bsp_wrapper_current_convert(
             u16_phase_b, g_st_current.u16_offset_b, p_phase_b)) ||
        (BSP_WRAPPER_CURRENT_OK != bsp_wrapper_current_convert(
             u16_phase_c, g_st_current.u16_offset_c, p_phase_c)))
    {
        return BSP_WRAPPER_CURRENT_ERROR;
    }

    return BSP_WRAPPER_CURRENT_OK;
}
//************************* Function Definitions end *************************//
