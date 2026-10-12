/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file sin_lut.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief 共享sin/cos查表模块: 1024点LUT + 位掩码取模, 供Park/反Park等复用.
 *
 * Processing flow:
 *
 *   LUT以const常量形式预嵌入Flash, 开机即可用, 直接调 sin_lut_sincos().
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef SIN_LUT_H
#define SIN_LUT_H

//********************************Includes***********************************//

#include <stdint.h>

//********************************Includes***********************************//


//*******************************Enumerations********************************//

typedef enum
{
    SIN_LUT_OK       = 0,       /* 查表成功           */
    SIN_LUT_ERR_NULL = 1         /* 空指针             */
} sin_lut_status_t;

//*******************************Enumerations********************************//


/**
 * @brief 查LUT同时得到sin(θ)和cos(θ).
 *
 * @param[in]  theta : 电角度 (rad), 支持负值与多圈环绕.
 * @param[out] p_sin : sin(θ) 输出指针.
 * @param[out] p_cos : cos(θ) 输出指针.
 *
 * @return sin_lut_status_t : 函数执行状态.
 *
 */
sin_lut_status_t sin_lut_sincos(
    const float      theta,
    float    * const p_sin,
    float    * const p_cos );


#endif /* SIN_LUT_H */
