/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file Clarke.c
 *
 * @par dependencies
 * - stdint.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief Clarke变换(等幅值): 把三相静止坐标系电流Ia/Ib/Ic投影到α-β两相正交坐标系.
 *
 * ============================================================================
 *                              调用方式
 * ============================================================================
 *
 *   前提: 中性点无接地, 三相电流满足 Ia + Ib + Ic = 0.
 *         因此只需采两相(Ia/Ib), 第三相由约束反推.
 *
 *   // 1. ADC采样完成中断中, 把三相电流校准成浮点(A).
 *   float i_a = ...;        // U相采样电流
 *   float i_b = ...;        // V相采样电流
 *
 *   // 2. 调用变换
 *   float i_alpha;
 *   float i_beta;
 *   if (CLARKE_OK == clarke_transform(i_a, i_b, &i_alpha, &i_beta))
 *   {
 *       // 把 i_alpha / i_beta 送去Park变换
 *   }
 *
 *   公式(等幅值):
 *     Iα = Ia
 *     Iβ = (Ia + 2·Ib) / √3        (等价于 (Ib - Ic) / √3)
 *
 *   另有clarke_transform_abc()重载: 若已采三相, 不依赖 Ia+Ib+Ic=0 的约束,
 *   直接按 Iβ = (Ib - Ic) / √3 计算, 对三相不平衡/采样失配更鲁棒.
 *
 * ============================================================================
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes***********************************//

#include <stddef.h>
#include <stdint.h>

//********************************Includes***********************************//


//*********************************Defines***********************************//

#define CLARKE_ONE_OVER_SQRT3   (0.5773502691896258f)   /* 1 / √3   */
#define CLARKE_ONE_THIRD        (0.3333333333333333f)   /* 1 / 3    */
#define CLARKE_TWO_THIRD        (0.6666666666666667f)   /* 2 / 3    */

//*********************************Defines***********************************//


//*******************************Enumerations********************************//

typedef enum
{
    CLARKE_OK       = 0,    /* 计算成功   */
    CLARKE_ERR_NULL = 1     /* 空指针     */
} clarke_status_t;

//*******************************Enumerations********************************//


/**
 * @brief Clarke变换(两相输入, 等幅值).
 *
 * Steps:
 *  1. 校验输出指针, 空指针直接返回错误.
 *  2. Iα = Ia, 直接赋值.
 *  3. Iβ = (Ia + 2·Ib) / √3, 利用常量乘代替除法.
 *
 * @param[in]  i_a       : U相电流 (A).
 * @param[in]  i_b       : V相电流 (A).
 * @param[out] p_i_alpha : α轴电流输出指针.
 * @param[out] p_i_beta  : β轴电流输出指针.
 *
 * @return clarke_status_t : 函数执行状态.
 *
 */
clarke_status_t clarke_transform(
    const float          i_a,
    const float          i_b,
    float    * const p_i_alpha,
    float    * const  p_i_beta )
{
    /* 1. 参数校验 */
    if ((NULL == p_i_alpha) || (NULL == p_i_beta))
    {
        return CLARKE_ERR_NULL;
    }

    /* 2. α轴与A相对齐, 直接等于Ia */
    *p_i_alpha = i_a;

    /* 3. Iβ = (Ia + 2·Ib) / √3 */
    *p_i_beta  = (i_a + (2.0f * i_b)) * CLARKE_ONE_OVER_SQRT3;

    return CLARKE_OK;
}


/**
 * @brief Clarke变换(三相输入, 等幅值, 不依赖 Ia+Ib+Ic=0).
 *
 * Steps:
 *  1. 校验输出指针, 空指针直接返回错误.
 *  2. Iα = (2·Ia - Ib - Ic) / 3, 对直流偏置更鲁棒.
 *  3. Iβ = (Ib - Ic) / √3.
 *
 * @param[in]  i_a       : U相电流 (A).
 * @param[in]  i_b       : V相电流 (A).
 * @param[in]  i_c       : W相电流 (A).
 * @param[out] p_i_alpha : α轴电流输出指针.
 * @param[out] p_i_beta  : β轴电流输出指针.
 *
 * @return clarke_status_t : 函数执行状态.
 *
 */
clarke_status_t clarke_transform_abc(
    const float          i_a,
    const float          i_b,
    const float          i_c,
    float    * const p_i_alpha,
    float    * const  p_i_beta )
{
    /* 1. 参数校验 */
    if ((NULL == p_i_alpha) || (NULL == p_i_beta))
    {
        return CLARKE_ERR_NULL;
    }

    /* 2. Iα = (2·Ia - Ib - Ic) / 3, 等价于 Ia - (Ib+Ic)/3 */
    *p_i_alpha = ((2.0f * i_a) - i_b - i_c) * CLARKE_ONE_THIRD;

    /* 3. Iβ = (Ib - Ic) / √3 */
    *p_i_beta  = (i_b - i_c) * CLARKE_ONE_OVER_SQRT3;

    return CLARKE_OK;
}
