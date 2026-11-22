/******************************************************************************
 * Copyright (C) 2026 YA. All Rights Reserved.
 * @file service_foc_config.h
 * @par dependencies 无。
 * @author YA
 * @brief 两台电机的初始化参数，宏通过初始化形参传入。
 * Processing flow: 修改参数 -> 初始化状态 -> 校验并初始化 PI。
 * @version V1.0
 * @note UTF-8，四空格缩进。参数须按实际电机填写。
 *****************************************************************************/
#ifndef SERVICE_FOC_CONFIG_H
#define SERVICE_FOC_CONFIG_H

//****************************** Defines begin *******************************//
//-------------------- 任务调度 begin --------------------//
#define FOC_M1_SPEED_DIV       (20U) /* 20kHz 每20拍执行，即1kHz。 */
#define FOC_M1_POSITION_DIV    (20U)          /* 20kHz每20拍算位置环，即1kHz。*/
#define FOC_M1_SPEED_PHASE     (19U) /* 第19拍计算平均速度及速度环。 */
#define FOC_M1_DMA_DIV         (4U)  /* DMA 独立保持5kHz采样。 */
#define FOC_M1_DMA_PHASE       (2U)           /* 每4拍中，第2拍启动DMA。      */
#define FOC_M1_POSITION_PHASE  (19U)          /* 每20拍中，第19拍算位置环。   */
//-------------------- 任务调度 end --------------------//
//-------------------- 电机 1 参数 begin --------------------//
/* 电机 1 基础参数。 */
#define FOC_M1_PWM_PERIOD           (17000U)     /* PWM 周期计数 */
#define FOC_M1_BUS_VOLTAGE_V        (12.0f)      /* 母线电压/V */
#define FOC_M1_POLE_PAIRS           (7U)         /* 极对数 */
#define FOC_M1_ENCODER_TIMEOUT_US   (1000U)       /* 反馈超时/us */
#define FOC_M1_PWM_PERIOD_S         (0.00005f)   /* PWM 周期/s，按定时器填写 */
//FOC_M1_ENCODER_DIRECTION                编码器方向
//FOC_M1_ELECTRICAL_ZERO_DEG              电角度零偏/度,需要电角度里标定service_foc_zero_load
#define FOC_M1_PHASE_OVERCURRENT_A  (3.5f)       //保护电流大小,超过判定短路

/* 电机 1 电流环：位置式 PI，输出及积分限幅单位为 V。 */
#define FOC_M1_CONTROL_PERIOD_S      (0.00005f)       /* 控制周期/s */

/* d/q 轴共用配置，PI 运行状态各自独立。
 * 两台电机的积分方式由 current_loop.c 内的编译期开关统一选择。 */
#define FOC_M1_CURRENT_KP           (0.5f) /* 比例增益 */
#define FOC_M1_CURRENT_KI           (10.f) /* 积分增益 */
/* 放开轴限幅至母线/SQRT(3)，最终合成电压仍由电流环矢量限幅。 */
#define FOC_M1_CURRENT_OUT_MAX_V \
    (FOC_M1_BUS_VOLTAGE_V * 0.57735026919f)
#define FOC_M1_CURRENT_INT_MAX_V    FOC_M1_CURRENT_OUT_MAX_V

/* 独立速度环调试给定；位置模式下由位置环产生目标速度。 */

/* 电机 1 速度环：输入机械角速度，输出 q 轴电流目标。 */
#define FOC_M1_SPEED_KP             (0.02f)       /* 速度比例增益 */
#define FOC_M1_SPEED_KI             (0.04f)      /* 速度积分增益 */
#define FOC_M1_SPEED_PERIOD_S \
    (FOC_M1_CONTROL_PERIOD_S * FOC_M1_SPEED_DIV)
#define FOC_M1_SPEED_OUT_LIMIT_A    (3.0f)       //速度环输出最大:目标最大电流

/* 位置微分滤波时间常数/s；Kd=0 时跳过微分计算。 */
#define FOC_M1_POSITION_D_RC_S (0.002f)

/* 电机 1 位置环：输入机械角度/rad，输出角速度目标/rad/s。 */
#define FOC_M1_POSITION_KP (4.0f)                /* 比例增益 */
#define FOC_M1_POSITION_KI (0.0f)                /* 积分增益 */
#define FOC_M1_POSITION_KD (0.0f)                /* 微分增益 */
#define FOC_M1_POSITION_PERIOD_S \
    (FOC_M1_CONTROL_PERIOD_S * FOC_M1_POSITION_DIV)
#define FOC_M1_POSITION_OUT_LIMIT_RAD_S (10.0f)  //位置环最大输出:目标最大速度
//-------------------- 电机 1 参数 end --------------------//

//-------------------- 电机 2 参数 begin --------------------//
/* 电机 2 基础参数。 */
#define FOC_M2_PWM_PERIOD           (17000U)     /* PWM 周期计数 */
#define FOC_M2_BUS_VOLTAGE_V        (0.0f)       /* 母线电压/V */
#define FOC_M2_POLE_PAIRS           (0U)         /* 极对数 */
#define FOC_M2_ENCODER_DIRECTION    (0)          /* 编码器方向，填 1 或 -1 */
#define FOC_M2_ENCODER_TIMEOUT_US   (0U)         /* 反馈超时/us */
#define FOC_M2_ELECTRICAL_ZERO_DEG  (0.0f)       /* 电角度零偏/度 */
#define FOC_M2_PWM_PERIOD_S         (0.0f)       /* PWM 周期/s，按定时器填写 */

/* 电机 2 电流环：位置式 PI，输出及积分限幅单位为 V。 */
#define FOC_M2_CONTROL_PERIOD_S      (0.0f)           /* 控制周期/s */

#define FOC_M2_D_KP                  (0.0f)           /* 比例增益 */
#define FOC_M2_D_KI                  (0.0f)           /* 积分增益 */
#define FOC_M2_D_OUT_MAX_V           (1.0f) /* 输出限幅/V，最大母线电压/√3 */
#define FOC_M2_D_INT_MAX_V           (1.0f) /* 积分限幅/V，最大母线电压/√3 */

#define FOC_M2_Q_KP                  (0.0f)           /* 比例增益 */
#define FOC_M2_Q_KI                  (0.0f)           /* 积分增益 */
#define FOC_M2_Q_OUT_MAX_V           (1.0f) /* 输出限幅/V，最大母线电压/√3 */
#define FOC_M2_Q_INT_MAX_V           (1.0f) /* 积分限幅/V，最大母线电压/√3 */

/* 电机 2 速度环：输入机械角速度，输出 q 轴电流目标。 */
#define FOC_M2_SPEED_KP             (0.0f)       /* 速度比例增益 */
#define FOC_M2_SPEED_KI             (0.0f)       /* 速度积分增益 */
#define FOC_M2_SPEED_PERIOD_S       (0.0f)       /* 速度环周期/s */
/* 速度环输出目标电流上限/A。 */
#define FOC_M2_SPEED_OUT_LIMIT_A (0.0f)
/* 实测相电流过流阈值/A，按硬件及波动实测填写；0 禁止运行和标定。 */
#define FOC_M2_PHASE_OVERCURRENT_A (0.0f)

/* 电机 2 位置环：输入机械角度/rad，输出角速度目标/rad/s。 */
#define FOC_M2_POSITION_KP (0.0f) /* 比例增益 */
#define FOC_M2_POSITION_KI (0.0f) /* 积分增益 */
#define FOC_M2_POSITION_KD (0.0f) /* 微分增益 */
#define FOC_M2_POSITION_PERIOD_S (0.0f) /* 位置环周期/s */
/* 位置环输出角速度限幅/rad/s。 */
#define FOC_M2_POSITION_OUT_LIMIT_RAD_S (0.0f)
//-------------------- 电机 2 参数 end --------------------//
//-------------------- 标定与停稳参数 begin --------------------//
/* 按当前台架多次日志留余量；等待超时不是静止判定阈值。 */
#define FOC_M1_CAL_ALIGN_VOLTAGE_V        (1.0f)  /* 标定对齐电压矢量幅值/V */
#define FOC_M1_STOP_TIMEOUT_MS            (5000U) /* 停稳总超时/ms */
#define FOC_M1_STOP_DMA_TIMEOUT_MS        (20U)   /* 等待编码器传输结束超时/ms */
#define FOC_M1_STOP_DIAGNOSTIC_MS         (100U)  /* 诊断时长/ms，当前未使用 */
#define FOC_M1_STOP_SPEED_RAD_S           (5.0f)  /* 静止机械角速度上限/rad/s，>0 */
#define FOC_M1_STOP_POSITION_COUNTS       (4U)    /* 稳定窗口位置峰峰值/计数，<2048 */
#define FOC_M1_STOP_STABLE_MS             (1000U) /* 持续静止时间/ms，须非零 */
#define FOC_M1_STOP_DECAY_MS              (1000U) /* 衰减等待/ms，与静止时间相加 */
#define FOC_M1_STOP_MAX_SAMPLE_GAP_US     (5000U) /* 停稳采样间隔上限/us */
#define FOC_M1_ZERO_MIN_A                 (2020U) /* A 相 ADC 下限，实测最低2039 */
#define FOC_M1_ZERO_MIN_B                 (2010U) /* B 相 ADC 下限，实测最低2029 */
#define FOC_M1_ZERO_MIN_C                 (2075U) /* C 相 ADC 下限，实测最低2094 */
#define FOC_M1_ZERO_MAX_A                 (2070U) /* A 相 ADC 上限，实测最高2048 */
#define FOC_M1_ZERO_MAX_B                 (2065U) /* B 相 ADC 上限，实测最高2046 */
#define FOC_M1_ZERO_MAX_C                 (2120U) /* C 相 ADC 上限，实测最高2100 */
#define FOC_M1_ZERO_MAX_SPAN_COUNTS       (24U)   /* ADC 峰峰值上限，实测最大17 */
#define FOC_M1_ZERO_MAX_DRIFT_COUNTS      (4U)    /* 半窗口均值差上限/ADC计数 */
#define FOC_M1_ZERO_MAX_SAMPLE_GAP_US     (500U)  /* 零偏采样间隔上限/us */
//-------------------- 标定与停稳参数 end --------------------//
//******************************* Defines end ********************************//
#endif /* SERVICE_FOC_CONFIG_H */
