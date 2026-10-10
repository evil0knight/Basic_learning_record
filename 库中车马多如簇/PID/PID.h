/******************************************************************************
 * Copyright (C) 2026 Fangzhou Technology, Inc. or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file PID.h
 *
 * @par dependencies
 * - stdbool.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 通用单环 PID,位置式/增量式二选一;功能开关即字段。
 *
 * Processing flow:
 *
 *     pid_init -> (loop) pid_calc [-> pid_sat_feedback] -> ...
 *
 * @version V3.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef PID_H
#define PID_H

//********************************Includes*********************************//

#include <stdbool.h>
#include <stdint.h>

//********************************Includes*********************************//

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- 枚举 ---------------- */

typedef enum {
    PID_OK = 0,
    PID_ERR_NULL,       /* 空指针 */
    PID_ERR_PARAM,      /* 参数非法或非有限数 */
    PID_ERR_STATE       /* 未初始化或无有效采样 */
} pid_status_t;

typedef enum {
    PID_POSITION  = 0,  /* 位置式 */
    PID_INCREMENT = 1   /* 增量式(内部累加,输出 u(k)) */
} pid_type_t;

/* ---------------- 配置 ----------------
 * 可选字段默认 0 / false 即关闭,填数或置 true 即启用。
 */
typedef struct {
    /* 基础(必填) */
    pid_type_t  e_type;
    float       f_kp;
    float       f_ki;
    float       f_kd;
    float       f_dt;                   /* 固定控制周期,秒,必须 > 0 */

    /* 限幅与滤波(0 关闭) */
    float       f_out_max;              /* 输出对称限幅 */
    float       f_int_max;              /* 积分对称限幅(位置式) */
    float       f_dead_band;            /* 误差死区 */
    float       f_i_sep_err;            /* 积分分离阈值 */
    float       f_vsi_a;                /* 变速积分过渡宽度 */
    float       f_vsi_b;                /* 变速积分全积分阈值 */
    float       f_d_rc;                 /* 微分低通时间常数 */
    float       f_out_rc;               /* 输出低通时间常数 */

    /* 结构改进(false 关闭) */
    bool        b_d_on_measure;         /* 微分先行 */
    bool        b_p_on_measure;         /* 比例先行 */
    bool        b_trapezoid_i;          /* 梯形积分 */

    /* 堵转检测(count 为 0 关闭) */
    uint32_t    u32_stall_count;
    float       f_stall_out_ratio;      /* [0,1] */
    float       f_stall_target_min;
    float       f_stall_err_ratio;
} pid_cfg_t;

/* ---------------- 运行时 ---------------- */

typedef struct {
    pid_cfg_t   s_cfg;

    /* 核心历史 */
    float       f_integral;             /* 位置式累加积分 */
    float       f_integral_prev;        /* 本周期之前的积分 */
    float       f_integral_term;        /* 本周期积分增量 */
    float       f_proportional;         /* 比例先行累加 */
    float       f_err_1;
    float       f_err_2;
    float       f_measure_1;
    float       f_measure_2;
    float       f_d_1;                  /* 上次微分输出,滤波用 */
    float       f_out;                  /* 当前输出 u(k) */
    float       f_out_1;                /* 上次输出,滤波用 */

    /* 堵转 */
    uint32_t    u32_stall_cnt;
    bool        b_stalled;              /* 用户可直接读 */

    /* 状态标志 */
    bool        b_init;
    bool        b_has_sample;
    bool        b_sat_pending;
} pid_t;

/* ---------------- 接口 ---------------- */

/**
 * @brief 校验配置并初始化状态。
 *
 * Steps:
 *  1. 检查指针与配置合法性。
 *  2. 清空运行时历史并记录配置副本。
 *
 * @param[out] p_pid : 控制器状态地址。
 * @param[in]  p_cfg : 已填好的配置。
 *
 * @return pid_status_t : 成功或错误码。
 */
pid_status_t pid_init(pid_t * const p_pid, const pid_cfg_t * const p_cfg);

/**
 * @brief 复位历史,保留原配置。
 *
 * Steps:
 *  1. 检查指针。
 *  2. 清空所有历史并保留 s_cfg。
 *
 * @param[in,out] p_pid : 控制器状态地址。
 *
 * @return pid_status_t : 成功或错误码。
 */
pid_status_t pid_reset(pid_t * const p_pid);

/**
 * @brief 本周期计算一次控制量。
 *
 * Steps:
 *  1. 校验入参、初始化标志与数值有限性。
 *  2. 按 e_type 分发位置式或增量式,内部按字段开关启用改进。
 *  3. 处理抗饱和、输出滤波、限幅、堵转判定并提交历史。
 *
 * @param[in,out] p_pid     : 已初始化的状态。
 * @param[in]     f_target  : 目标值。
 * @param[in]     f_measure : 反馈值。
 * @param[out]    p_out     : 控制量输出地址,与状态独立。
 *
 * @return pid_status_t : 成功或错误码,失败时状态与输出不变。
 */
pid_status_t pid_calc(
    pid_t * const   p_pid,
    const float     f_target,
    const float     f_measure,
    float * const   p_out);

/**
 * @brief 执行器实际输出被缩放时的饱和反馈(仅位置式有效)。
 *
 * Steps:
 *  1. 校验指针、初始化与采样标志、缩放范围 [0,1]。
 *  2. 增量式直接返回;位置式在同向饱和时回退本周期积分。
 *  3. 每次计算最多消费一次,重复调用无效。
 *
 * @param[in,out] p_pid   : 刚完成 pid_calc 的状态。
 * @param[in]     f_scale : 实际输出缩放比例 [0,1],1 表示未饱和。
 *
 * @return pid_status_t : 成功或错误码。
 */
pid_status_t pid_sat_feedback(pid_t * const p_pid, const float f_scale);

#ifdef __cplusplus
}
#endif

#endif /* PID_H */
