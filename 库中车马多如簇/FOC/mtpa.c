/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file mtpa.c
 *
 * @par dependencies
 * - stddef.h
 * - stdint.h
 * - math.h
 *
 * @author YA
 *
 * @brief FOC最大转矩电流比(MTPA): 转矩指令 → 最优 (id_ref, iq_ref).
 *        作为"中间件"挂在速度环与电流环之间: 挂上则走MTPA, 不挂则走id=0控制,
 *        电流环本身接口完全不动. 内部提供"公式计算法"与"查表法"二选一.
 *
 * ============================================================================
 *                        中间件挂载位置(FOC三环)
 * ============================================================================
 *
 *   速度环输出语义取决于下游是否挂了MTPA:
 *
 *     [不挂 MTPA / id=0 控制]
 *       ω_ref ─▶[speed_loop]─▶ iq_ref ─────────────────────▶[current_loop]
 *                              id_ref = 0 ──────────────────▶
 *
 *     [挂上 MTPA 作为中间件]
 *       ω_ref ─▶[speed_loop]─▶ te_ref ─▶[ mtpa_apply ]─▶ id_ref ─▶[current_loop]
 *                                                      └▶ iq_ref ─▶
 *
 *   换句话说: 挂了MTPA, 速度环的输出就从"q轴电流指令"升级为"转矩指令",
 *   由MTPA一步到位算出轨迹上的 (id, iq); 不挂就按原本的 id=0 控制走.
 *   电流环只负责吃 (id_ref, iq_ref), 根本不关心上游怎么来的.
 *
 * ============================================================================
 *                              原理与接口
 * ============================================================================
 *
 *   原理:
 *     IPMSM 转矩  Te = 1.5·p·iq·[ψf + (Ld - Lq)·id]
 *     Lagrange 极值 ⇒ MTPA 轨迹:
 *       id = [ψf - √(ψf² + 4·Δ²·iq²)] / (2Δ),  Δ = Lq - Ld
 *     SPMSM (Δ=0) 退化: id = 0, iq = Te / (1.5·p·ψf).
 *
 *   // 1. 启动时(main里一次): 填好参数, 内部顺手把LUT预算出来
 *   mtpa_init();
 *
 *   // 2. 挂载点: 在速度环输出到电流环输入之间, 一行搞定
 *   mtpa_apply(te_ref, &id_ref, &iq_ref);
 *     // 内部通过 MTPA_USE_LUT 宏切换:
 *     //   未定义 → mtpa_formula_calc()  (公式法, 对温漂/饱和敏感时首选)
 *     //   已定义 → mtpa_lut_calc()      (查表法, 省MCU算力)
 *
 *   // 3. 外部电流圆/弱磁限幅(field_weakening给出iq_max), 再送电流环
 *   if (iq_ref > iq_max) { iq_ref = iq_max; }
 *   current_loop_calc(id_ref, iq_ref, ...);
 *
 *   // 搜索法(在线扰动id观测Te/|Is|): 收敛慢且易被噪声干扰, 不在此实现.
 *
 * ============================================================================
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes***********************************//

#include <math.h>
#include <stddef.h>
#include <stdint.h>

//********************************Includes***********************************//


//*********************************Defines***********************************//

#define MTPA_POLE_PAIRS   (4)                   /* 极对数 p                 */
#define MTPA_PSI_F        (0.080f)              /* 永磁磁链 (Wb)            */
#define MTPA_L_D          (0.0004f)             /* d轴电感 (H)              */
#define MTPA_L_Q          (0.0012f)             /* q轴电感 (H), IPMSM>Ld    */
#define MTPA_I_MAX        (20.0f)               /* 相电流峰值上限 (A)       */

#define MTPA_LUT_SIZE     (32)                  /* 查表点数, 2的幂便于二分  */
#define MTPA_K_T          (1.5f * MTPA_POLE_PAIRS)   /* 转矩常数前缀         */

/* 中间件默认走公式法, 需要查表法时在工程层 define 这个宏 */
/* #define MTPA_USE_LUT */

//*********************************Defines***********************************//


//*******************************Enumerations********************************//

typedef enum
{
    MTPA_OK       = 0,
    MTPA_ERR_NULL = 1
} mtpa_status_t;

//*******************************Enumerations********************************//


//*****************************Static Variables******************************//

/* 预生成LUT: te_ref ↗ 对应 id_ref / iq_ref.
 * 开机 mtpa_init() 填完, 运行期只读. */
static float g_s_lut_te[MTPA_LUT_SIZE];
static float g_s_lut_id[MTPA_LUT_SIZE];
static float g_s_lut_iq[MTPA_LUT_SIZE];
static float g_s_te_max;                        /* 本电机MTPA可达最大转矩  */

//*****************************Static Variables******************************//


//**************************Static Function Decls****************************//

static float mtpa_id_on_locus(const float f_iq);
static float mtpa_torque_of(const float f_id, const float f_iq);

//**************************Static Function Decls****************************//


/**
 * @brief MTPA轨迹上的 id(iq): 由iq反推轨迹对应的id.
 *
 *   公式: id = [ψf - √(ψf² + 4·Δ²·iq²)] / (2Δ),  Δ = Lq - Ld.
 *   Δ=0 (SPMSM) 时直接返回0.
 *
 * @param[in] f_iq : q轴电流 (A).
 *
 * @return float : 轨迹上对应的 id (A, 通常≤0).
 *
 */
static float mtpa_id_on_locus(const float f_iq)
{
    const float delta = MTPA_L_Q - MTPA_L_D;
    float       disc;

    if (1e-9f > fabsf(delta))
    {
        return 0.0f;
    }

    disc = sqrtf((MTPA_PSI_F * MTPA_PSI_F)
               + (4.0f * delta * delta * f_iq * f_iq));
    return (MTPA_PSI_F - disc) / (2.0f * delta);
}


/**
 * @brief 由 (id, iq) 算电磁转矩.
 *
 * @param[in] f_id : d轴电流 (A).
 * @param[in] f_iq : q轴电流 (A).
 *
 * @return float : 电磁转矩 (N·m).
 *
 */
static float mtpa_torque_of(const float f_id, const float f_iq)
{
    return MTPA_K_T * ((MTPA_PSI_F * f_iq)
                     + ((MTPA_L_D - MTPA_L_Q) * f_id * f_iq));
}


/**
 * @brief 初始化MTPA模块: 预生成LUT, 记录可达最大转矩.
 *
 * Steps:
 *  1. 以iq从0到I_max均匀取点, 落在MTPA轨迹上得(id, iq, Te).
 *  2. 存入三张平行数组, g_s_te_max记录最后一点(最大iq对应的Te).
 *
 * @return mtpa_status_t : 函数执行状态.
 *
 */
mtpa_status_t mtpa_init(void)
{
    uint32_t i;
    float    iq;
    float    id;

    for (i = 0u; i < MTPA_LUT_SIZE; i++)
    {
        iq = ((float)i / (float)(MTPA_LUT_SIZE - 1u)) * MTPA_I_MAX;
        id = mtpa_id_on_locus(iq);

        g_s_lut_iq[i] = iq;
        g_s_lut_id[i] = id;
        g_s_lut_te[i] = mtpa_torque_of(id, iq);
    }

    g_s_te_max = g_s_lut_te[MTPA_LUT_SIZE - 1u];

    return MTPA_OK;
}


/**
 * @brief 公式计算法: 给定转矩, 二分iq收敛到MTPA轨迹上, 回解id.
 *
 * Steps:
 *  1. 参数校验与转矩夹到[0, Te_max].
 *  2. 二分 iq ∈ [0, I_max]:
 *       取 mid, id = mtpa_id_on_locus(mid), 算 Te(mid).
 *       Te偏小 → iq上拉, Te偏大 → iq下拉, 收敛到 Te_ref.
 *  3. 输出 (id, iq); 若Te_ref<0, 对称翻转iq符号.
 *
 * @param[in]  f_te_ref : 转矩指令 (N·m), 支持双向.
 * @param[out] p_id_ref : 输出d轴参考电流指针 (A).
 * @param[out] p_iq_ref : 输出q轴参考电流指针 (A).
 *
 * @return mtpa_status_t : 函数执行状态.
 *
 */
mtpa_status_t mtpa_formula_calc(
    const float      f_te_ref,
    float    * const p_id_ref,
    float    * const p_iq_ref )
{
    float    te_abs;
    float    sign;
    float    lo;
    float    hi;
    float    mid;
    float    id;
    uint32_t i;

    /* 1. 校验 */
    if ((NULL == p_id_ref) || (NULL == p_iq_ref))
    {
        return MTPA_ERR_NULL;
    }

    sign   = (f_te_ref < 0.0f) ? -1.0f : 1.0f;
    te_abs = fabsf(f_te_ref);
    if (te_abs > g_s_te_max)
    {
        te_abs = g_s_te_max;
    }

    /* 2. 二分 iq */
    lo = 0.0f;
    hi = MTPA_I_MAX;
    for (i = 0u; i < 32u; i++)                  /* 32次 ≈ 1e-9 相对精度   */
    {
        mid = 0.5f * (lo + hi);
        id  = mtpa_id_on_locus(mid);
        if (mtpa_torque_of(id, mid) < te_abs)
        {
            lo = mid;
        }
        else
        {
            hi = mid;
        }
    }
    mid = 0.5f * (lo + hi);
    id  = mtpa_id_on_locus(mid);

    /* 3. 输出 */
    *p_id_ref = id;                             /* id轴与转矩方向无关       */
    *p_iq_ref = sign * mid;

    return MTPA_OK;
}


/**
 * @brief 查表法: 预生成LUT里二分查Te_ref, 线性插值得 (id, iq).
 *
 * Steps:
 *  1. 参数校验与转矩夹到[0, Te_max].
 *  2. 对 g_s_lut_te 二分找到 Te_ref 所在区间 [k, k+1].
 *  3. 线性插值得到 id_ref / iq_ref.
 *  4. 若Te_ref<0, 翻转iq符号.
 *
 * @param[in]  f_te_ref : 转矩指令 (N·m), 支持双向.
 * @param[out] p_id_ref : 输出d轴参考电流指针 (A).
 * @param[out] p_iq_ref : 输出q轴参考电流指针 (A).
 *
 * @return mtpa_status_t : 函数执行状态.
 *
 */
mtpa_status_t mtpa_lut_calc(
    const float      f_te_ref,
    float    * const p_id_ref,
    float    * const p_iq_ref )
{
    float    te_abs;
    float    sign;
    uint32_t lo;
    uint32_t hi;
    uint32_t mid;
    float    ratio;
    float    id;
    float    iq;

    /* 1. 校验与夹限 */
    if ((NULL == p_id_ref) || (NULL == p_iq_ref))
    {
        return MTPA_ERR_NULL;
    }

    sign   = (f_te_ref < 0.0f) ? -1.0f : 1.0f;
    te_abs = fabsf(f_te_ref);
    if (te_abs >= g_s_te_max)
    {
        *p_id_ref = g_s_lut_id[MTPA_LUT_SIZE - 1u];
        *p_iq_ref = sign * g_s_lut_iq[MTPA_LUT_SIZE - 1u];
        return MTPA_OK;
    }

    /* 2. 二分找区间: te 单调递增 */
    lo = 0u;
    hi = MTPA_LUT_SIZE - 1u;
    while ((hi - lo) > 1u)
    {
        mid = (lo + hi) >> 1u;
        if (g_s_lut_te[mid] <= te_abs)
        {
            lo = mid;
        }
        else
        {
            hi = mid;
        }
    }

    /* 3. 线性插值 */
    ratio = (te_abs           - g_s_lut_te[lo])
          / (g_s_lut_te[hi]   - g_s_lut_te[lo]);
    id    = g_s_lut_id[lo] + (ratio * (g_s_lut_id[hi] - g_s_lut_id[lo]));
    iq    = g_s_lut_iq[lo] + (ratio * (g_s_lut_iq[hi] - g_s_lut_iq[lo]));

    /* 4. 输出 */
    *p_id_ref = id;
    *p_iq_ref = sign * iq;

    return MTPA_OK;
}


/**
 * @brief MTPA 中间件统一入口: 挂在速度环与电流环之间, 一行完成转矩→(id, iq).
 *
 *   内部按 MTPA_USE_LUT 宏切换具体算法, 对调用者透明:
 *     - 未定义 MTPA_USE_LUT → 走 mtpa_formula_calc (默认, 公式法)
 *     - 已定义 MTPA_USE_LUT → 走 mtpa_lut_calc    (查表法)
 *
 *   不想启用 MTPA 时, 整个 mtpa_apply 调用链不编译/不调用即可,
 *   退回原本 id=0 控制: id_ref = 0, iq_ref = te_ref / (1.5·p·ψf).
 *
 * @param[in]  f_te_ref : 转矩指令 (N·m), 由速度环输出.
 * @param[out] p_id_ref : 输出d轴参考电流指针 (A), 送电流环.
 * @param[out] p_iq_ref : 输出q轴参考电流指针 (A), 送电流环.
 *
 * @return mtpa_status_t : 函数执行状态.
 *
 */
mtpa_status_t mtpa_apply(
    const float      f_te_ref,
    float    * const p_id_ref,
    float    * const p_iq_ref )
{
#ifdef MTPA_USE_LUT
    return mtpa_lut_calc(f_te_ref, p_id_ref, p_iq_ref);
#else
    return mtpa_formula_calc(f_te_ref, p_id_ref, p_iq_ref);
#endif
}
