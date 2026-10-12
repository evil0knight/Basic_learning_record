/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file Park.c
 *
 * @par dependencies
 * - sin_lut.h
 * - stdint.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief Park变换: 把静止α-β坐标系投影到随转子旋转的d-q坐标系.
 *
 * ============================================================================
 *                              调用方式
 * ============================================================================
 *
 *   公式:
 *     Id =  Iα·cos(θe) + Iβ·sin(θe)
 *     Iq = -Iα·sin(θe) + Iβ·cos(θe)
 *
 *   sin LUT已以 const 常量形式预嵌入Flash (sin_lut.c), 开机即可用, 无需init.
 *
 *   // 1. 简单版: 内部查LUT拿sin/cos.
 *   park_transform(i_alpha, i_beta, theta_e, &i_d, &i_q);
 *
 *   // 2. 预算版: 外部已经查过表, 直接传sin/cos进来, 与反Park共用一次查表.
 *   float sin_t, cos_t;
 *   sin_lut_sincos(theta_e, &sin_t, &cos_t);
 *   park_transform_sc  (i_alpha, i_beta, sin_t, cos_t, &i_d,     &i_q    );
 *   inverse_park_sc    (v_d,     v_q,    sin_t, cos_t, &v_alpha, &v_beta );
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
    PARK_OK       = 0,  /* 计算成功       */
    PARK_ERR_NULL = 1,  /* 空指针         */
    PARK_ERR_LUT  = 2   /* LUT查表失败    */
} park_status_t;

//*******************************Enumerations********************************//


/**
 * @brief Park变换(内部查LUT).
 *
 * Steps:
 *  1. 校验输出指针, 空指针直接返回错误.
 *  2. 调 sin_lut_sincos 一次性拿到 sin/cos.
 *  3. 按Park公式求出 Id, Iq.
 *
 * @param[in]  i_alpha : α轴电流 (A).
 * @param[in]  i_beta  : β轴电流 (A).
 * @param[in]  theta_e : 电角度 (rad).
 * @param[out] p_i_d   : d轴电流输出指针.
 * @param[out] p_i_q   : q轴电流输出指针.
 *
 * @return park_status_t : 函数执行状态.
 *
 */
park_status_t park_transform(
    const float      i_alpha,
    const float       i_beta,
    const float      theta_e,
    float    * const    p_i_d,
    float    * const    p_i_q )
{
    float            sin_theta;
    float            cos_theta;
    sin_lut_status_t lut_s;

    /* 1. 参数校验 */
    if ((NULL == p_i_d) || (NULL == p_i_q))
    {
        return PARK_ERR_NULL;
    }

    /* 2. 查表 */
    lut_s = sin_lut_sincos(theta_e, &sin_theta, &cos_theta);
    if (SIN_LUT_OK != lut_s)
    {
        return PARK_ERR_LUT;
    }

    /* 3. Id =  Iα·cos(θ) + Iβ·sin(θ) */
    /*    Iq = -Iα·sin(θ) + Iβ·cos(θ) */
    *p_i_d =  (i_alpha * cos_theta) + (i_beta * sin_theta);
    *p_i_q = -(i_alpha * sin_theta) + (i_beta * cos_theta);

    return PARK_OK;
}


/**
 * @brief Park变换(预算sin/cos版).
 *
 * 适合FOC主循环: 与反Park共用同一个θe, 外部调一次 sin_lut_sincos()
 * 把sin/cos分别喂给 park_transform_sc 和 inverse_park_sc, 省一次查表.
 *
 * Steps:
 *  1. 校验输出指针, 空指针直接返回错误.
 *  2. 按Park公式求出 Id, Iq.
 *
 * @param[in]  i_alpha   : α轴电流 (A).
 * @param[in]  i_beta    : β轴电流 (A).
 * @param[in]  sin_theta : sin(θe), 外部查表得到.
 * @param[in]  cos_theta : cos(θe), 外部查表得到.
 * @param[out] p_i_d     : d轴电流输出指针.
 * @param[out] p_i_q     : q轴电流输出指针.
 *
 * @return park_status_t : 函数执行状态.
 *
 */
park_status_t park_transform_sc(
    const float      i_alpha,
    const float       i_beta,
    const float    sin_theta,
    const float    cos_theta,
    float    * const    p_i_d,
    float    * const    p_i_q )
{
    /* 1. 参数校验 */
    if ((NULL == p_i_d) || (NULL == p_i_q))
    {
        return PARK_ERR_NULL;
    }

    /* 2. Id =  Iα·cos(θ) + Iβ·sin(θ) */
    /*    Iq = -Iα·sin(θ) + Iβ·cos(θ) */
    *p_i_d =  (i_alpha * cos_theta) + (i_beta * sin_theta);
    *p_i_q = -(i_alpha * sin_theta) + (i_beta * cos_theta);

    return PARK_OK;
}
