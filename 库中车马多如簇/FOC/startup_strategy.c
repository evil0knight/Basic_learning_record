/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file startup_strategy.c
 *
 * @par dependencies
 * - stddef.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief FOC启动策略模块: 对齐 → I-F开环 → 平滑切换到闭环, 产出(id_ref,iq_ref,θ).
 *
 * ============================================================================
 *                              为什么需要启动策略
 * ============================================================================
 *
 *   电机静止时转子位置未知, 直接进闭环FOC会:
 *     - 无法确定施加电压的方向 → 可能反转/抖动/堵转
 *     - 无传感器观测器在零速/低速下反电动势≈0, 根本观测不到角度
 *
 *   所以启动必须分阶段走:
 *     1. 对齐:   强制把转子拉到θ=0的已知位置, 建立基准
 *     2. I-F:    角度开环旋转, 像"拖拉"一样把转子拽起来, 加速到观测器可用
 *     3. 切换:   把控制角度从"我自己积的"平滑过渡到"观测器估的", 不产生转矩冲击
 *     4. 闭环:   启动完成, 本模块退出, 外部用速度/位置环接管
 *
 *   关键认识:
 *     - 电流环全程都是闭环的, 从阶段1就在跑.
 *     - "开环/闭环"指的是"角度来源", 不是"电流控制".
 *     - 本模块只产出 (id_ref, iq_ref, θ), 不碰ADC/Clarke/Park/SVPWM, 职责干净.
 *
 * ============================================================================
 *                              调用方式
 * ============================================================================
 *
 *   // 1. 启动时
 *   startup_init();
 *
 *   // 2. PWM中断里, 每周期调一次, 拿到当前该用的 id_ref/iq_ref/theta
 *   startup_out_t so;
 *   startup_step(DT, theta_obs, obs_ready, &so);
 *
 *   if (STARTUP_STATE_CLOSEDLOOP == so.state) {
 *       // 启动完成, 外部接管: iq_ref来自速度环, theta用观测器的
 *       so.id_ref = 0.0f;
 *       so.iq_ref = iq_from_speed_loop;
 *       so.theta  = theta_obs;
 *   }
 *
 *   // 3. 把so.id_ref/iq_ref/theta送进电流环
 *   current_loop_calc(so.id_ref, so.iq_ref, id, iq, omega, &vd, &vq);
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

/* ---- 数学常量 ---- */
#define STARTUP_TWO_PI             (6.2831853071795864769f) /* 2π            */

/* ---- 阶段1: 对齐预定位 ----
 * 施加 iq=STARTUP_ALIGN_IQ, θ=0 持续 STARTUP_ALIGN_TIME_S 秒,
 * 让转子磁极被 "拉" 到d轴方向(θ=0电角度)上. 时长要够长,
 * 保证带负载/有静摩擦时也能稳定对齐. 电流不能大,否则会过流/发热.
 */
#define STARTUP_ALIGN_IQ           (2.0f)   /* 对齐电流 (A), ≈20%额定       */
#define STARTUP_ALIGN_TIME_S       (0.3f)   /* 对齐时长 (s), 100~500ms典型  */

/* ---- 阶段2: I-F开环启动 ----
 * 频率从 F_START 以 ACCEL 斜坡升到 F_SWITCH, 角度=∫2π·f dt.
 * 电流幅值恒定, 转子被旋转磁场 "拽" 着转.
 *   - F_SWITCH太低: 观测器反电动势不够, 切换后失步
 *   - F_SWITCH太高: 开环时间长, 效率低且会抖
 *   - ACCEL太快:   转子跟不上, 失步
 */
#define STARTUP_IF_IQ              (3.0f)   /* I-F阶段q轴电流 (A), 20~50%额定 */
#define STARTUP_IF_F_START         (1.0f)   /* 起始电频率 (Hz)                */
#define STARTUP_IF_F_SWITCH        (10.0f)  /* 切换到闭环的频率 (Hz), ≈10%额定 */
#define STARTUP_IF_ACCEL           (50.0f)  /* 频率斜坡 (Hz/s), 10~100典型     */

/* ---- 阶段3: 平滑切换 ----
 * 每周期 α += STEP, 控制角 θ = (1-α)·θ_开环 + α·θ_观测.
 * STEP=0.01 → 100个控制周期切完 (10kHz下 10ms).
 */
#define STARTUP_TRANSITION_STEP    (0.01f)  /* α每周期增量, 1→100周期切完     */

//*********************************Defines***********************************//


//*******************************Enumerations********************************//

/* 启动状态机的四个阶段, 按顺序推进, 不可逆回退 */
typedef enum
{
    STARTUP_STATE_ALIGNMENT   = 0,  /* 对齐预定位              */
    STARTUP_STATE_OPENLOOP_IF = 1,  /* I-F开环递增频率         */
    STARTUP_STATE_TRANSITION  = 2,  /* 角度平滑切换到观测器    */
    STARTUP_STATE_CLOSEDLOOP  = 3   /* 闭环, 启动模块不再接管  */
} startup_state_t;

/* 函数返回状态 */
typedef enum
{
    STARTUP_OK       = 0,
    STARTUP_ERR_NULL = 1
} startup_status_t;

//*******************************Enumerations********************************//


//*********************************Structs***********************************//

/* step函数的输出, 外部拿去喂给电流环 */
typedef struct
{
    float           id_ref;    /* d轴电流目标 (A), 启动阶段恒0  */
    float           iq_ref;    /* q轴电流目标 (A), 产生牵引转矩  */
    float           theta;     /* 送进反Park的电角度 (rad)      */
    startup_state_t state;     /* 当前阶段, 调用方据此决定是否接管 */
} startup_out_t;

//*********************************Structs***********************************//


//*****************************Static Variables******************************//

/* 文件内闭包: 对外只暴露init/step两个函数, 内部状态封装在这里 */
static startup_state_t g_s_state       = STARTUP_STATE_ALIGNMENT;
static float           g_s_align_timer = 0.0f;   /* 对齐阶段累计时长 (s)   */
static float           g_s_freq_hz     = 0.0f;   /* I-F当前电频率 (Hz)     */
static float           g_s_theta_open  = 0.0f;   /* 开环积分出来的电角度(rad) */
static float           g_s_alpha       = 0.0f;   /* 平滑切换混合系数 [0,1] */

//*****************************Static Variables******************************//


/**
 * @brief 初始化启动状态机, 清零所有累加器.
 *
 * 作用:
 *   把状态拉回阶段1(对齐), 频率/角度/α全归零,
 *   使模块进入 "刚上电" 的初始状态.
 *
 *   需要重新启动电机(停机后再转)时也要调用一次.
 *
 * @return startup_status_t : 函数执行状态.
 *
 */
startup_status_t startup_init(void)
{
    g_s_state       = STARTUP_STATE_ALIGNMENT;
    g_s_align_timer = 0.0f;
    g_s_freq_hz     = STARTUP_IF_F_START;
    g_s_theta_open  = 0.0f;
    g_s_alpha       = 0.0f;

    return STARTUP_OK;
}


/**
 * @brief 启动策略单步计算: 根据当前阶段产出(id_ref, iq_ref, theta).
 *
 * Steps:
 *  1. 参数校验.
 *  2. id_ref 永远给0 (启动阶段不弱磁, 全部转矩分量给到q轴).
 *  3. 按状态分支:
 *     - ALIGNMENT   : theta=0, iq=对齐电流, 累计到时长后切OPENLOOP_IF.
 *     - OPENLOOP_IF : 频率斜坡+积分出theta_open, 到切换频率且观测器可靠切TRANSITION.
 *     - TRANSITION  : theta = (1-α)·theta_open + α·theta_obs, α→1后切CLOSEDLOOP.
 *     - CLOSEDLOOP  : 不再接管, state返回出去让外部自己接.
 *  4. 填充输出.
 *
 * @param[in]  dt        : 控制周期 (s), 一般=PWM周期, 用于频率积分和α增量.
 * @param[in]  theta_obs : 观测器估算的电角度 (rad), 仅TRANSITION/CLOSEDLOOP时用.
 * @param[in]  obs_ready : 观测器是否已可靠(反电动势够大且速度稳), 0=未就绪 1=就绪.
 * @param[out] p_out     : 输出结果指针.
 *
 * @return startup_status_t : 函数执行状态.
 *
 */
startup_status_t startup_step(
    const float             dt,
    const float      theta_obs,
    const uint8_t    obs_ready,
    startup_out_t  * const p_out )
{
    /* 参数校验: 空指针直接返回 */
    if (NULL == p_out)
    {
        return STARTUP_ERR_NULL;
    }

    /* 启动全程 id=0, 把全部电流分量投到q轴产生转矩 */
    p_out->id_ref = 0.0f;

    switch (g_s_state)
    {
        /* ================================================================ */
        /*  阶段1: 对齐预定位                                                */
        /*                                                                  */
        /*  做什么: 固定施加 θ=0, iq=对齐电流, 让转子被磁场拉到d轴位置.      */
        /*                                                                  */
        /*  原理:  d轴定义为 "N极指向" 的方向. 在d轴注入正电流会在定子产生   */
        /*          指向d轴正方向的磁场, 转子磁极被这个磁场吸引, 稳定后      */
        /*          转子N极就对齐到了d轴 ⇒ 此时转子电角度=0, 已知!           */
        /*                                                                  */
        /*  退出:  累计时长到 ALIGN_TIME_S 后无条件进入阶段2.                */
        /* ================================================================ */
        case STARTUP_STATE_ALIGNMENT:
        {
            p_out->iq_ref = STARTUP_ALIGN_IQ;
            p_out->theta  = 0.0f;              /* 强制θ=0, 定住磁场方向 */

            g_s_align_timer += dt;             /* 累计对齐时长 */
            if (g_s_align_timer >= STARTUP_ALIGN_TIME_S)
            {
                /* 对齐够久, 切换到I-F开环启动, 重置频率和开环角度 */
                g_s_state      = STARTUP_STATE_OPENLOOP_IF;
                g_s_theta_open = 0.0f;
                g_s_freq_hz    = STARTUP_IF_F_START;
            }
            break;
        }

        /* ================================================================ */
        /*  阶段2: I-F开环加速                                               */
        /*                                                                  */
        /*  做什么: 自己生成一个旋转磁场, 频率从低到高斜坡上升, 把转子"拽"起 */
        /*                                                                  */
        /*  原理:  电流幅值恒定(iq=IF_IQ), 角度自己积分 θ=∫2π·f dt.          */
        /*          转子的真实位置可能滞后于磁场, 但只要转矩够、加速够慢,    */
        /*          转子会被磁场拉着转起来 (类似同步电机的异步启动).         */
        /*                                                                  */
        /*  为什么必须: 零速时无传感器观测器算不出角度 (反电动势≈0),         */
        /*              必须靠开环把速度先拉到观测器可用的区间.              */
        /*                                                                  */
        /*  退出:  频率到 F_SWITCH 且观测器已可靠 → 进入阶段3.               */
        /* ================================================================ */
        case STARTUP_STATE_OPENLOOP_IF:
        {
            /* 频率斜坡上升: f += a·dt, 到目标就停止增长 */
            if (g_s_freq_hz < STARTUP_IF_F_SWITCH)
            {
                g_s_freq_hz += STARTUP_IF_ACCEL * dt;
            }

            /* 角度积分: θ += ω·dt = 2π·f·dt */
            g_s_theta_open += STARTUP_TWO_PI * g_s_freq_hz * dt;

            /* θ 归一化到 [0, 2π), 防止浮点溢出丢精度 */
            if (g_s_theta_open >= STARTUP_TWO_PI)
            {
                g_s_theta_open -= STARTUP_TWO_PI;
            }

            p_out->iq_ref = STARTUP_IF_IQ;
            p_out->theta  = g_s_theta_open;

            /* 切换条件: 速度够(反电动势够) + 观测器自己也说没问题 */
            if ((g_s_freq_hz >= STARTUP_IF_F_SWITCH) && (0U != obs_ready))
            {
                g_s_state = STARTUP_STATE_TRANSITION;
                g_s_alpha = 0.0f;              /* 从纯开环角开始切 */
            }
            break;
        }

        /* ================================================================ */
        /*  阶段3: 角度平滑切换                                              */
        /*                                                                  */
        /*  做什么: 控制角 = (1-α)·开环角 + α·观测角, α从0平滑升到1.         */
        /*                                                                  */
        /*  为什么不能瞬切: 开环角和观测角之间几乎必然有误差 (开环滞后角),   */
        /*                  瞬间切会让电流矢量角度突变 → 转矩冲击 → 失步.    */
        /*                                                                  */
        /*  注意: 开环积分继续推, 保证"退路"存在; 一旦切换过程中观测器掉线   */
        /*         就能回滚(本实现未做回滚, 真实项目建议补上).               */
        /*                                                                  */
        /*  退出:  α=1时, 控制角完全等于观测角 → 进入阶段4.                  */
        /* ================================================================ */
        case STARTUP_STATE_TRANSITION:
        {
            /* 继续推开环积分, 维持开环侧频率 */
            g_s_theta_open += STARTUP_TWO_PI * g_s_freq_hz * dt;
            if (g_s_theta_open >= STARTUP_TWO_PI)
            {
                g_s_theta_open -= STARTUP_TWO_PI;
            }

            /* α 线性从0升到1, 控制切换速度 */
            g_s_alpha += STARTUP_TRANSITION_STEP;
            if (g_s_alpha >= 1.0f)
            {
                g_s_alpha = 1.0f;
                g_s_state = STARTUP_STATE_CLOSEDLOOP;  /* 切换完毕 */
            }

            p_out->iq_ref = STARTUP_IF_IQ;
            /* 加权混合: 一开始偏开环, 结束时完全是观测器 */
            p_out->theta  = ((1.0f - g_s_alpha) * g_s_theta_open)
                          + (g_s_alpha         * theta_obs);
            break;
        }

        /* ================================================================ */
        /*  阶段4: 闭环接管(本模块退出)                                     */
        /*                                                                  */
        /*  做什么: 把state标为CLOSEDLOOP, 让调用方自己拿观测器角度和速度环  */
        /*          输出的iq_ref覆盖掉本模块的输出.                          */
        /*                                                                  */
        /*  这里给的默认值仅是兜底(iq=0, θ=θ_obs), 外部务必覆盖.             */
        /* ================================================================ */
        case STARTUP_STATE_CLOSEDLOOP:
        default:
        {
            p_out->iq_ref = 0.0f;
            p_out->theta  = theta_obs;
            break;
        }
    }

    /* 把当前阶段吐出去, 调用方据此判断是否接管 */
    p_out->state = g_s_state;

    return STARTUP_OK;
}
