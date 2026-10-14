/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file field_weakening.c
 *
 * @par dependencies
 * - PID.h
 * - stddef.h
 * - math.h
 *
 * @author YA
 *
 * @brief FOC弱磁控制: 电压闭环PI, 过压时注入负id_ref削弱磁链, 同步更新电流圆iq上限.
 *        作为"中间件"挂在上游 (id=0 / MTPA) 与电流环之间: 挂上则按合成电压自动弱磁,
 *        不挂则直通上游 (id, iq), 电流环本身接口完全不动.
 *
 * ============================================================================
 *                        中间件挂载位置(FOC三环)
 * ============================================================================
 *
 *   弱磁必须拿到上一拍电流环真实输出的 (vd, vq) 才能判断是否过压,
 *   所以挂载点横跨两拍:
 *
 *     [不挂 FW]   上游 ──▶ id_up/iq_up ─────────────────▶[current_loop]─▶vd/vq
 *
 *     [挂上 FW 作为中间件]
 *       上游 ──▶ id_up/iq_up ─▶[ fw_apply ]─▶ id_ref/iq_ref ─▶[current_loop]─▶vd/vq
 *                                   ▲                                        │
 *                                   └──────────  上一拍 vd/vq  ◀────── 存起来 ┘
 *
 *   内部一步到位:
 *     1) 用上一拍 vd/vq 更新电压PI → 本拍弱磁候选 id_fw (≤0).
 *     2) id_ref = min(id_up, id_fw)       // 同时满足 MTPA/id=0 要求与弱磁要求
 *     3) iq_max = √(I_max² - id_ref²)     // 电流圆剩余额度
 *     4) iq_ref = clamp(iq_up, ±iq_max).
 *
 *   不挂弱磁: 整条 fw_apply 调用链不编译, id_ref/iq_ref 直通上游, 高速时q轴会
 *   饱和失控, 加不上去; 挂上即自动在基速以上注入负id给q轴腾电压.
 *
 * ============================================================================
 *                              原理与接口
 * ============================================================================
 *
 *   公式:
 *     U_out = √(v_d² + v_q²)
 *     U_max = margin · V_bus / √3
 *     PI(target=U_max, measure=U_out) 输出 id_fw:
 *       过压 err<0 → id_fw变负 → 削弱磁链;
 *       欠压 err>0 → 外部clamp到0, 同时 pid_sat_feedback 退积分, 禁止增磁.
 *     id_fw下界由 f_out_max 保证, 防止永磁退磁.
 *
 *   // 1. 启动时(main里一次)
 *   field_weakening_init();
 *
 *   // 2. 挂载点: 每拍电流环前, 一行完成"弱磁合并 + 电流圆限幅"
 *   fw_apply(g_last_vd, g_last_vq, v_bus, I_MAX,
 *            id_up, iq_up, &id_ref, &iq_ref);
 *   current_loop_calc(id_ref, iq_ref, ..., &vd, &vq);
 *   g_last_vd = vd;  g_last_vq = vq;        // 存给下一拍的fw_apply
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

#include "PID.h"

//********************************Includes***********************************//


//*********************************Defines***********************************//

#define FW_TS          (0.0001f)        /* 10kHz, 与电流环同周期            */
#define FW_V_MARGIN    (0.95f)          /* 5%电压余量                       */
#define FW_SQRT3_INV   (0.577350269f)   /* 1/√3                             */
#define FW_ID_MAX      (15.0f)          /* 弱磁电流绝对值上限(A), 退磁硬限  */
#define FW_KP          (0.01f)          /* 需实测整定                       */
#define FW_KI          (0.5f)           /* 需实测整定                       */

//*********************************Defines***********************************//


//*******************************Enumerations********************************//

typedef enum
{
    FW_OK       = 0,
    FW_ERR_NULL = 1
} fw_status_t;

//*******************************Enumerations********************************//


//*****************************Static Variables******************************//

static pid_t g_s_fw_pid;

/* 对称限幅 ±FW_ID_MAX, 正半轴由外部clamp到0并通过pid_sat_feedback退积分. */
static const pid_cfg_t g_s_fw_cfg = {
    .e_type    = PID_POSITION,
    .f_kp      = FW_KP,
    .f_ki      = FW_KI,
    .f_dt      = FW_TS,
    .f_out_max = FW_ID_MAX,
    .f_int_max = FW_ID_MAX
};

//*****************************Static Variables******************************//


/**
 * @brief 初始化弱磁PI.
 *
 * @return fw_status_t : 函数执行状态.
 *
 */
fw_status_t field_weakening_init(void)
{
    (void)pid_init(&g_s_fw_pid, &g_s_fw_cfg);
    return FW_OK;
}


/**
 * @brief 复位弱磁历史(停机/切模式时用), 保留PI配置.
 *
 * @return fw_status_t : 函数执行状态.
 *
 */
fw_status_t field_weakening_reset(void)
{
    (void)pid_reset(&g_s_fw_pid);
    return FW_OK;
}


/**
 * @brief 弱磁单次计算: (v_d, v_q, V_bus, I_max) → (id_ref, iq_max).
 *
 * Steps:
 *  1. 参数校验.
 *  2. 合成电压 U_out = √(v_d²+v_q²), 母线上限 U_max = margin·V_bus/√3.
 *  3. PI以 target=U_max, measure=U_out 计算, 输出即弱磁电流候选.
 *  4. 外部clamp到 [-Id_fw_max, 0]: 正值→0 并调sat_feedback退积分, 禁止增磁.
 *  5. 电流圆剩余额度 iq_max = √(I_max² - id²), 保护q轴不超相电流上限.
 *
 * @param[in]  v_d       : d轴电压(本拍电流环输出) (V).
 * @param[in]  v_q       : q轴电压(本拍电流环输出) (V).
 * @param[in]  v_bus     : 母线电压(实测, 允许波动) (V).
 * @param[in]  i_max     : 相电流峰值上限 (A).
 * @param[out] p_id_ref  : 下一拍d轴电流指令指针(≤0) (A).
 * @param[out] p_iq_max  : 下一拍q轴电流可用上限指针(≥0) (A).
 *
 * @return fw_status_t : 函数执行状态.
 *
 */
fw_status_t field_weakening_calc(
    const float            v_d,
    const float            v_q,
    const float          v_bus,
    const float          i_max,
    float    * const  p_id_ref,
    float    * const  p_iq_max )
{
    float u_out;
    float u_max;
    float pi_out;
    float iq_sq;

    /* 1. 参数校验 */
    if ((NULL == p_id_ref) || (NULL == p_iq_max))
    {
        return FW_ERR_NULL;
    }

    /* 2. 合成电压与母线上限 */
    u_out = sqrtf((v_d * v_d) + (v_q * v_q));
    u_max = FW_V_MARGIN * v_bus * FW_SQRT3_INV;

    /* 3. PI: err = u_max - u_out
     *    过压(u_out>u_max): err<0 → pi_out负 → 削弱磁链
     *    欠压(u_out<u_max): err>0 → pi_out正 → 下面截0并退积分 */
    (void)pid_calc(&g_s_fw_pid, u_max, u_out, &pi_out);

    /* 4. 非对称clamp到 [-Id_fw_max, 0] */
    if (0.0f < pi_out)
    {
        pi_out = 0.0f;
        (void)pid_sat_feedback(&g_s_fw_pid, 0.0f);
    }
    /* 下界由 f_out_max 保证, 无需再处理 */

    *p_id_ref = pi_out;

    /* 5. 电流圆剩余: iq_max = √(I_max² - id²) */
    iq_sq = (i_max * i_max) - (pi_out * pi_out);
    if (0.0f > iq_sq)
    {
        iq_sq = 0.0f;
    }
    *p_iq_max = sqrtf(iq_sq);

    return FW_OK;
}


/**
 * @brief 弱磁中间件统一入口: 在电流环前一行完成"合并上游 + 弱磁 + 电流圆限幅".
 *
 * Steps:
 *  1. 参数校验.
 *  2. 调 field_weakening_calc, 由上一拍 vd/vq 算本拍 id_fw 和 iq_max.
 *  3. 合并: id_out = min(id_up, id_fw).
 *       - 低速(欠压): id_fw=0, 完全听上游(MTPA/id=0);
 *       - 高速(过压): id_fw<id_up, 弱磁接管, 更负的那一个胜出.
 *  4. iq_max 用 id_out(而非 id_fw) 重算一次, 保证限幅圆对的是最终id.
 *  5. iq_out = clamp(iq_up, ±iq_max).
 *
 *   不启用弱磁时整个 fw_apply 调用链不编译, (id_ref, iq_ref) = (id_up, iq_up)
 *   直通电流环, 系统自动退回"无弱磁"行为.
 *
 * @param[in]  v_d_prev  : 上一拍电流环输出的d轴电压 (V).
 * @param[in]  v_q_prev  : 上一拍电流环输出的q轴电压 (V).
 * @param[in]  v_bus     : 母线电压(实测, 允许波动) (V).
 * @param[in]  i_max     : 相电流峰值上限 (A).
 * @param[in]  id_up     : 上游(MTPA/id=0)给的id候选 (A).
 * @param[in]  iq_up     : 上游给的iq候选 (A).
 * @param[out] p_id_out  : 最终d轴指令指针, 送电流环 (A).
 * @param[out] p_iq_out  : 最终q轴指令指针, 送电流环 (A).
 *
 * @return fw_status_t : 函数执行状态.
 *
 */
fw_status_t fw_apply(
    const float          v_d_prev,
    const float          v_q_prev,
    const float             v_bus,
    const float             i_max,
    const float             id_up,
    const float             iq_up,
    float    * const     p_id_out,
    float    * const     p_iq_out )
{
    float id_fw;
    float iq_max_tmp;
    float id_out;
    float iq_sq;
    float iq_max_final;

    /* 1. 校验 */
    if ((NULL == p_id_out) || (NULL == p_iq_out))
    {
        return FW_ERR_NULL;
    }

    /* 2. 更新弱磁PI, 拿到本拍 id_fw */
    (void)field_weakening_calc(v_d_prev, v_q_prev, v_bus, i_max,
                               &id_fw, &iq_max_tmp);
    (void)iq_max_tmp;                   /* 下面按 id_out 重算            */

    /* 3. id_out = min(id_up, id_fw): 更负者胜 */
    id_out = (id_fw < id_up) ? id_fw : id_up;

    /* 4. 电流圆剩余按最终 id_out 重算 */
    iq_sq = (i_max * i_max) - (id_out * id_out);
    if (0.0f > iq_sq)
    {
        iq_sq = 0.0f;
    }
    iq_max_final = sqrtf(iq_sq);

    /* 5. iq_out = clamp(iq_up, ±iq_max_final) */
    if (iq_up > iq_max_final)
    {
        *p_iq_out = iq_max_final;
    }
    else if (iq_up < -iq_max_final)
    {
        *p_iq_out = -iq_max_final;
    }
    else
    {
        *p_iq_out = iq_up;
    }

    *p_id_out = id_out;

    return FW_OK;
}
