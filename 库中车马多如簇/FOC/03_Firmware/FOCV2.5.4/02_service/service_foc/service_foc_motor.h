/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file service_foc_motor.h
 * @par dependencies
 * - ../../04_platform/platform_mcu/adc/inc/adc_port.h
 * @author YA
 * @brief 双电机三相采样、PWM 和转子状态数据定义。
 *
 * Processing flow:
 * 仅定义单台电机的数据布局，供后续两台电机各自实例化。
 *
 * @version V1.0
 * @note 四个空格缩进，中文注释，UTF-8 编码。
 *****************************************************************************/

#ifndef SERVICE_FOC_MOTOR_H
#define SERVICE_FOC_MOTOR_H

//****************************** Includes begin ******************************//
#include "../../04_platform/platform_mcu/adc/inc/adc_port.h"
#include <stdint.h>
#include "../../06_middlewares/algorithm/FOC/inc/current_loop.h"
#include "../../06_middlewares/algorithm/FOC/inc/speed_loop.h"
#include "../../06_middlewares/algorithm/FOC/inc/position_loop.h"
//******************************* Includes end *******************************//

//******************************* Enums begin ********************************//
/** @brief 电机输出状态；枚举存储宽度由编译器配置决定。 */
typedef enum
{
    SERVICE_FOC_OUTPUT_STOP_REQUEST = 0, /* 目标停止，尚未确认关断或静止。 */
    SERVICE_FOC_OUTPUT_STOPPING,         /* 已关断，等待停稳。 */
    SERVICE_FOC_OUTPUT_STOPPED,          /* 已确认静止及电流衰减。 */
    SERVICE_FOC_OUTPUT_STARTING,         /* 正在准备开启。 */
    SERVICE_FOC_OUTPUT_ENABLED           /* 输出已开启。 */
} en_service_foc_output_state_t;
//******************************** Enums end *********************************//

//****************************** Structs begin *******************************//
/** @brief 配置及硬件映射；控制回调停用时修改。 */
typedef struct
{
    uint8_t u8_pole_pairs;              /* 极对数；0 表示尚未配置。 */
    int8_t i8_encoder_direction;        /* 方向：+1/-1；0 表示未配置。 */
    float f_electrical_zero_deg;        /* 电角度零偏，单位度。 */
    uint32_t u32_encoder_timeout_us;    /* 编码器角度样本最大年龄，单位 us。 */
    en_core_adc_instance_t en_adc_instance; /* 电机 1 用 ADC2，电机 2 用 ADC1。 */
    uint16_t u16_pwm_period;            /* PWM 周期计数 PER。 */
    float f_pwm_period_s;               /* PWM 周期，单位 s。 */
    float f_inv_bus_voltage; /* 母线倒数，初始化计算，单位 1/V。 */

    float f_phase_overcurrent_a; /* 实测相电流过流阈值，单位 A。 */

    /* 控制器配置：电流环、速度环、位置环。 */
    st_current_pi_cfg_t st_d_pi_cfg;     /* d 轴电流 PI 配置。 */
    st_current_pi_cfg_t st_q_pi_cfg;     /* q 轴电流 PI 配置。 */

} st_service_foc_config_t;

/** @brief 输出及有效性状态；保持原有中断访问约束。 */
typedef struct
{
    volatile en_service_foc_output_state_t en_output_state; /* 输出状态。 */
    volatile uint8_t u8_control_valid; /* 本次电流环计算成功。 */
    uint8_t u8_electrical_zero_valid; /* Flash 或完整标定有效。 */
    uint8_t u8_control_initialized; /* 独立 PI 初始化完成。 */
    uint8_t u8_hardware_ready; /* 反馈和采样链已准备。 */
    uint8_t u8_angle_update_pending; /* 电角度待转换。 */
    uint8_t u8_encoder_valid; /* 编码器反馈未过期。 */
} st_service_foc_status_t;

/** @brief 机械、电角度及母线反馈。 */
typedef struct
{
    float f_mechanical_angle_deg; /* 机械角度，[0, 360)，单位度。 */
    float f_mechanical_velocity_rad_s; /* 机械角速度，单位 rad/s。 */
    float f_electrical_angle_deg; /* 电角度，[0, 360)，单位度。 */
    float f_bus_voltage_v; /* 母线电压，单位 V，待配置。 */
} st_service_foc_feedback_t;

/** @brief 控制目标，电流单位为 A。 */
typedef struct
{
    float f_id_ref_a; /* d 轴目标电流，单位 A。 */
    float f_iq_ref_a; /* q 轴目标电流，单位 A。 */
} st_service_foc_target_t;

/** @brief 三相采样及 PWM 比较值。 */
typedef struct
{
    float f_current_a; /* A/U 相电流，单位 A。 */
    float f_current_b; /* B/V 相电流，单位 A。 */
    float f_current_c; /* C/W 相电流，单位 A。 */
    uint16_t u16_pwm_cmp_a; /* A/U 相 PWM 比较计数，范围 [0, PER]。 */
    uint16_t u16_pwm_cmp_b; /* B/V 相 PWM 比较计数，范围 [0, PER]。 */
    uint16_t u16_pwm_cmp_c; /* C/W 相 PWM 比较计数，范围 [0, PER]。 */
} st_service_foc_three_phase_t;

/** @brief 电流环运行对象及本周期计算结果。 */
typedef struct
{
    float f_i_alpha_a; /* alpha 电流，单位 A。 */
    float f_i_beta_a; /* beta 电流，单位 A。 */
    float f_id_a; /* d 轴反馈电流，单位 A。 */
    float f_iq_a; /* q 轴反馈电流，单位 A。 */
    float f_sin_electrical; /* 本周期电角度正弦值。 */
    float f_cos_electrical; /* 本周期电角度余弦值。 */
    float f_vd_v; /* d 轴输出电压，单位 V。 */
    float f_vq_v; /* q 轴输出电压，单位 V。 */
    float f_v_alpha_v; /* alpha 电压，单位 V。 */
    float f_v_beta_v; /* beta 电压，单位 V。 */
    st_speed_loop_t st_speed_loop; /* 机械速度 PI，输出 Iq/A。 */
    st_position_loop_t st_position_loop; /* 单圈位置控制器及目标。 */
    float f_speed_ref_rad_s; /* 位置环输出，速度环保持使用。 */
    uint32_t u32_schedule_tick; /* 20kHz 时基，位置分频周期内拍号。 */
    st_current_loop_t st_current_loop; /* 本电机独立的 PI 运行状态。 */
} st_service_foc_control_t;

/**
 * @brief 单台电机状态，三相依次为 A/U、B/V、C/W。
 * @note 硬件映射依据 02_Hardware/FOCV2.0.pdf。
 *
 * ------------------------- 电机 1 begin -------------------------
 * 电流：IA1/IB1/IC1 -> PC0/PC1/PC2 -> ADC2 IN6/7/8，Rank 1/2/3。
 * PWM ：A/B/C -> PA8/PA10/PB12 -> HRTIM TA1/TB1/TC1。
 * -------------------------- 电机 1 end --------------------------
 *
 * ------------------------- 电机 2 begin -------------------------
 * 电流：IA2/IB2/IC2 -> PA0/PA1/PA2 -> ADC1 IN1/2/3，Rank 1/2/3。
 * PWM ：D/E/F -> PB14/PC8/PC6 -> HRTIM TD1/TE1/TF1。
 * -------------------------- 电机 2 end --------------------------
 *
 * @note 配置极对数、方向并完成零偏标定后，电角度才有效。
 */
typedef struct
{
    uint8_t u8_motor_id;                   //电机编号
    st_service_foc_config_t st_config;     //电机参数、硬件映射及控制器配置
    st_service_foc_status_t st_status; /* 输出状态及初始化、标定和反馈有效标志。 */
    st_service_foc_feedback_t st_feedback; /* 机械角度、速度、电角度及母线电压反馈。 */
    st_service_foc_target_t st_target; /* d/q 轴目标电流。 */
    st_service_foc_three_phase_t st_phase; /* 三相电流及 PWM 比较计数。 */
    st_service_foc_control_t st_control;   // 中间过程变量。
} st_service_foc_motor_t;
//******************************* Structs end ********************************//

#endif /* SERVICE_FOC_MOTOR_H */

//******************************* End of File ********************************//
