/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file service_foc_init.h
 * @par dependencies
 * - service_foc.h
 * - service_foc_motor.h
 * @author YA
 * @brief 电机状态与 FOC 硬件链初始化接口。
 *
 * Processing flow:
 * 初始化两台电机状态 -> 初始化编码器、电流采样及 PWM 硬件链。
 *
 * @version V1.0
 * @note 四个空格缩进，UTF-8 编码。
 *****************************************************************************/

#ifndef SERVICE_FOC_INIT_H
#define SERVICE_FOC_INIT_H

//****************************** Includes begin ******************************//
#include "service_foc.h"
#include "service_foc_motor.h"
#include "service_foc_motor1.h"
#include "service_foc_motor2.h"
//******************************* Includes end *******************************//


//****************************** Structs begin *******************************//
/** @brief 单台电机的配置；运行状态由电机实例独立保存。 */
typedef struct
{
    uint8_t u8_motor_id; /* 电机编号，1 或 2。 */
    en_core_adc_instance_t en_adc_instance; /* 电机 1/2 对应 ADC2/ADC1。 */
    uint16_t u16_pwm_period; /* PWM 周期计数，须非零。 */
    float f_bus_voltage_v; /* 母线电压/V，0 表示待配置。 */
    uint8_t u8_pole_pairs; /* 极对数，0 表示待配置。 */
    int8_t i8_encoder_direction; /* 方向 +1/-1，0 表示待配置。 */
    uint32_t u32_encoder_timeout_us; /* 编码器反馈超时/us。 */
    float f_electrical_zero_deg; /* 默认电角度零偏/度。 */
    float f_control_period_s; /* 电流环周期/s。 */
    float f_pwm_period_s; /* PWM 周期/s。 */
    float f_phase_overcurrent_a; /* 实测相电流过流阈值/A，0 待配置。 */
    st_current_pi_cfg_t st_d_pi_cfg; /* d 轴 PI 配置，周期使用电流环周期。 */
    st_current_pi_cfg_t st_q_pi_cfg; /* q 轴 PI 配置，周期使用电流环周期。 */
    st_speed_loop_cfg_t st_speed_cfg; /* 周期由电流周期和分频推导。 */
    st_position_loop_cfg_t st_position_cfg;
    uint32_t u32_speed_div;
    uint32_t u32_position_div;
} st_service_foc_motor_config_t;
//******************************* Structs end ********************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 根据状态结构体与配置初始化单台电机软件。
 * @param[in,out] p_motor : 待初始化的电机状态。
 * @param[in] p_config : 对应电机的只读配置。
 * @return app_status_t : 参数、Flash 读取或控制器初始化结果。
 * @note 调用前须关闭输出并解除控制回调；不初始化硬件。
 */
app_status_t service_foc_software_init(
    st_service_foc_motor_t *p_motor,
    const st_service_foc_motor_config_t *p_config);

/**
 * @brief 建立采样链，阻塞停机并校准电流，保持驱动关闭。
 * @return app_status_t : 初始化结果；未确认阈值仅诊断，不放行输出。
 * @note 由初始化模块调用，控制回调须事先解除；ADC 须先内部校准。
 */
app_status_t service_foc_hardware_init(void);
//************************* Function Prototypes end *************************//

#endif /* SERVICE_FOC_INIT_H */

//******************************* End of File ********************************//
