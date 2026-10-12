/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file Inverse_Park.c
 *
 * @par dependencies
 * - sin_lut.h
 * - stdint.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 逆Park变换: 将旋转d-q坐标系下的电压投影回静止α-β坐标系.
 *
 * ============================================================================
 *                              调用方式
 * ============================================================================
 *
 *   公式:
 *     Vα = Vd·cos(θe) - Vq·sin(θe)
 *     Vβ = Vd·sin(θe) + Vq·cos(θe)
 *
 *   sin LUT已以 const 常量形式预嵌入Flash (sin_lut.c), 开机即可用, 无需init.
 *
 *   // 1. 简单版: 内部查LUT拿sin/cos.
 *   inverse_park(v_d, v_q, theta_e, &v_alpha, &v_beta);
 *
 *   // 2. 预算版: 外部已经查过表, 直接传sin/cos进来, Park与反Park共用.
 *   float sin_t, cos_t;
 *   sin_lut_sincos(theta_e, &sin_t, &cos_t);
 *   park_transform_sc    (i_alpha, i_beta, sin_t, cos_t, &i_d, &i_q);
 *   inverse_park_sc      (v_d,     v_q,    sin_t, cos_t, &v_alpha, &v_beta);
 *
 * ============================================================================
 *
 * @version V2.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes***********************************//

#include "sin_lut.h"

#include <stddef.h>
#include <stdint.h>

//********************************Includes***********************************//


//*******************************Enumerations********************************//

typedef enum
{
    INV_PARK_OK            = 0, /* 计算成功           */
    INV_PARK_ERR_NULL      = 1, /* 空指针             */
    INV_PARK_ERR_LUT       = 2  /* LUT查表失败        */
} inv_park_status_t;

//*******************************Enumerations********************************//


/**
 * @brief 逆Park变换(内部查LUT).
 *
 * Steps:
 *  1. 校验输出指针, 空指针直接返回错误.
 *  2. 调 sin_lut_sincos 一次性拿到 sin/cos.
 *  3. 按逆Park公式求出 Vα, Vβ.
 *
 * @param[in]  v_d       : d轴电压分量.
 * @param[in]  v_q       : q轴电压分量.
 * @param[in]  theta_e   : 电角度 (rad).
 * @param[out] p_v_alpha : α轴电压输出指针.
 * @param[out] p_v_beta  : β轴电压输出指针.
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
    float            sin_theta;
    float            cos_theta;
    sin_lut_status_t lut_s;

    /* 1. 参数校验 */
    if ((NULL == p_v_alpha) || (NULL == p_v_beta))
    {
        return INV_PARK_ERR_NULL;
    }

    /* 2. 查表 */
    lut_s = sin_lut_sincos(theta_e, &sin_theta, &cos_theta);
    if (SIN_LUT_OK != lut_s)
    {
        return INV_PARK_ERR_LUT;
    }

    /* 3. Vα = Vd·cos(θ) - Vq·sin(θ) */
    /*    Vβ = Vd·sin(θ) + Vq·cos(θ) */
    *p_v_alpha = (v_d * cos_theta) - (v_q * sin_theta);
    *p_v_beta  = (v_d * sin_theta) + (v_q * cos_theta);

    return INV_PARK_OK;
}


/**
 * @brief 逆Park变换(预算sin/cos版).
 *
 * 适合FOC主循环: 与Park共用同一个θe, 外部调一次 sin_lut_sincos()
 * 把sin/cos分别喂给 park_transform_sc 和 inverse_park_sc, 省一次查表.
 *
 * Steps:
 *  1. 校验输出指针, 空指针直接返回错误.
 *  2. 按逆Park公式求出 Vα, Vβ.
 *
 * @param[in]  v_d       : d轴电压分量.
 * @param[in]  v_q       : q轴电压分量.
 * @param[in]  sin_theta : sin(θe), 外部查表得到.
 * @param[in]  cos_theta : cos(θe), 外部查表得到.
 * @param[out] p_v_alpha : α轴电压输出指针.
 * @param[out] p_v_beta  : β轴电压输出指针.
 *
 * @return inv_park_status_t : 函数执行状态.
 *
 */
inv_park_status_t inverse_park_sc(
    const float          v_d,
    const float          v_q,
    const float    sin_theta,
    const float    cos_theta,
    float    * const p_v_alpha,
    float    * const  p_v_beta )
{
    /* 1. 参数校验 */
    if ((NULL == p_v_alpha) || (NULL == p_v_beta))
    {
        return INV_PARK_ERR_NULL;
    }

    /* 2. Vα = Vd·cos(θ) - Vq·sin(θ) */
    /*    Vβ = Vd·sin(θ) + Vq·cos(θ) */
    *p_v_alpha = (v_d * cos_theta) - (v_q * sin_theta);
    *p_v_beta  = (v_d * sin_theta) + (v_q * cos_theta);

    return INV_PARK_OK;
}
