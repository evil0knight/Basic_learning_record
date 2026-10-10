/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file Inverse_Park.c
 *
 * @par dependencies
 * - math.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 逆Park变换(查表法): 将旋转d-q坐标系下的电压分量投影回静止α-β坐标系.
 *
 * Processing flow:
 *
 *  1. 启动时调用 inverse_park_init() 预先把一个周期的sin值填入LUT.
 *  2. 实时控制中反复调用 inverse_park(), 内部只做一次乘法+两次查表+四次MAC.
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes***********************************//

#include <math.h>
#include <stdint.h>
#include <stddef.h>

//********************************Includes***********************************//


//*********************************Defines***********************************//

#define INV_PARK_LUT_BITS       (10U)                           /* 1024点  */
#define INV_PARK_LUT_SIZE       (1U << INV_PARK_LUT_BITS)
#define INV_PARK_LUT_MASK       (INV_PARK_LUT_SIZE - 1U)        /* 位掩码取模 */
#define INV_PARK_COS_OFFSET     (INV_PARK_LUT_SIZE >> 2)        /* π/2偏移  */
#define INV_PARK_TWO_PI         (6.2831853071795864769f)
#define INV_PARK_LUT_SCALE      ((float)INV_PARK_LUT_SIZE / INV_PARK_TWO_PI)

//*********************************Defines***********************************//


//*******************************Enumerations********************************//

typedef enum
{
    INV_PARK_OK                = 0,
    INV_PARK_ERR_NULL          = 1,
    INV_PARK_ERR_LUT_NOT_READY = 2
} inv_park_status_t;

//*******************************Enumerations********************************//


//*****************************Static Variables******************************//

static float   g_sin_lut[INV_PARK_LUT_SIZE];
static uint8_t g_lut_ready = 0U;

//*****************************Static Variables******************************//


/**
 * @brief 预计算sin查表, 启动时调用一次即可.
 *
 * @return inv_park_status_t : 函数执行状态.
 *
 */
inv_park_status_t inverse_park_init(void)
{
    uint32_t i;

    for (i = 0U; i < INV_PARK_LUT_SIZE; i++)
    {
        g_sin_lut[i] = sinf((INV_PARK_TWO_PI * (float)i)
                             / (float)INV_PARK_LUT_SIZE);
    }
    g_lut_ready = 1U;

    return INV_PARK_OK;
}


/**
 * @brief 逆Park变换(查表法).
 *
 * Steps:
 *  1. 判断输出指针与LUT就绪状态.
 *  2. 把电角度缩放到LUT索引, cos索引 = sin索引 + π/2偏移.
 *  3. 位掩码取模, 自动处理负角度和多圈环绕.
 *  4. 按逆Park公式求出V_alpha, V_beta.
 *
 * @param[in]  v_d       : d轴电压分量.
 * @param[in]  v_q       : q轴电压分量.
 * @param[in]  theta_e   : 电角度(单位: rad).
 * @param[out] p_v_alpha : 指向α轴电压输出的指针.
 * @param[out] p_v_beta  : 指向β轴电压输出的指针.
 *
 * @return inv_park_status_t : 函数执行状态.
 *
 */
inv_park_status_t inverse_park(
    const float          v_d,
    const float          v_q,
    const float      theta_e,
    float    * const p_v_alpha,
    float    * const  p_v_beta )
{
    int32_t  raw_idx;
    uint32_t sin_idx;
    uint32_t cos_idx;
    float    sin_theta;
    float    cos_theta;

    /* 参数校验: 空指针直接返回错误 */
    if ((NULL == p_v_alpha) || (NULL == p_v_beta))
    {
        return INV_PARK_ERR_NULL;
    }
    /* LUT未初始化, 禁止使用 */
    if (0U == g_lut_ready)
    {
        return INV_PARK_ERR_LUT_NOT_READY;
    }

    /* 电角度 → LUT索引; 对负值和>2π都靠位掩码环绕 */
    raw_idx   = (int32_t)(theta_e * INV_PARK_LUT_SCALE);
    sin_idx   = ((uint32_t)raw_idx) & INV_PARK_LUT_MASK;
    cos_idx   = ((uint32_t)(raw_idx + (int32_t)INV_PARK_COS_OFFSET))
                 & INV_PARK_LUT_MASK;

    sin_theta = g_sin_lut[sin_idx];
    cos_theta = g_sin_lut[cos_idx];

    /* V_alpha = V_d * cos(theta) - V_q * sin(theta) */
    /* V_beta  = V_d * sin(theta) + V_q * cos(theta) */
    *p_v_alpha = (v_d * cos_theta) - (v_q * sin_theta);
    *p_v_beta  = (v_d * sin_theta) + (v_q * cos_theta);

    return INV_PARK_OK;
}
