/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file service_foc_init.c
 *
 * @par dependencies
 * - service_foc_init.h
 * - ../../04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h
 * - gpio_port.h
 * - hrtim.h
 *
 * @author YA
 *
 * @brief 电机软件状态、控制器与共享采样硬件初始化。
 *
 * Processing flow:
 *
 * 分别初始化电机状态 -> 初始化控制器 -> 建立共享采样与 PWM 硬件链。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "service_foc_init.h"
#include "service_foc_zero_load/service_foc_zero_load.h"
#include "service_foc_config.h"
#include "../../04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h"
#include "../../04_platform/platform_mcu/gpio/inc/gpio_port.h"
#include "hrtim.h"
#include <float.h>
#include <math.h>
#include <string.h>
//******************************* Includes end *******************************//


//***************************** Constants begin ******************************//
/* 两台电机的软件初始化分别使用各自的配置。 */
static const st_service_foc_motor_config_t g_st_motor1_config =
{
    .u8_motor_id = 1U,
    .en_adc_instance = CORE_ADC2,
    .u16_pwm_period = FOC_M1_PWM_PERIOD,
    .f_bus_voltage_v = FOC_M1_BUS_VOLTAGE_V,
    .u8_pole_pairs = FOC_M1_POLE_PAIRS,
    .i8_encoder_direction = 0, /* 方向由有效标定记录加载。 */
    .u32_encoder_timeout_us = FOC_M1_ENCODER_TIMEOUT_US,
    .f_electrical_zero_deg = 0.0f, /* 零偏由有效标定记录加载。 */
    .f_control_period_s = FOC_M1_CONTROL_PERIOD_S,
    .f_pwm_period_s = FOC_M1_PWM_PERIOD_S,
    .st_d_pi_cfg =
    {
        .f_kp = FOC_M1_CURRENT_KP,
        .f_ki = FOC_M1_CURRENT_KI,
        .f_out_max = FOC_M1_CURRENT_OUT_MAX_V,
        .f_int_max = FOC_M1_CURRENT_INT_MAX_V,
    },
    .st_q_pi_cfg =
    {
        .f_kp = FOC_M1_CURRENT_KP,
        .f_ki = FOC_M1_CURRENT_KI,
        .f_out_max = FOC_M1_CURRENT_OUT_MAX_V,
        .f_int_max = FOC_M1_CURRENT_INT_MAX_V,
    },
    .f_phase_overcurrent_a = FOC_M1_PHASE_OVERCURRENT_A,
    .st_speed_cfg =
    {
        .f_kp = FOC_M1_SPEED_KP,
        .f_ki = FOC_M1_SPEED_KI,
        .f_out_max = FOC_M1_SPEED_OUT_LIMIT_A,
        .f_int_max = FOC_M1_SPEED_OUT_LIMIT_A,
    },
    .st_position_cfg =
    {
        .f_kp = FOC_M1_POSITION_KP,
        .f_ki = FOC_M1_POSITION_KI,
        .f_kd = FOC_M1_POSITION_KD,
        .f_out_max = FOC_M1_POSITION_OUT_LIMIT_RAD_S,
        .f_int_max = FOC_M1_POSITION_OUT_LIMIT_RAD_S,
        .f_d_rc = FOC_M1_POSITION_D_RC_S,
    },
    .u32_speed_div = FOC_M1_SPEED_DIV,
    .u32_position_div = FOC_M1_POSITION_DIV
};

static const st_service_foc_motor_config_t g_st_motor2_config =
{
    .u8_motor_id = 2U,
    .en_adc_instance = CORE_ADC1,
    .u16_pwm_period = FOC_M2_PWM_PERIOD,
    .f_bus_voltage_v = FOC_M2_BUS_VOLTAGE_V,
    .u8_pole_pairs = FOC_M2_POLE_PAIRS,
    .i8_encoder_direction = FOC_M2_ENCODER_DIRECTION,
    .u32_encoder_timeout_us = FOC_M2_ENCODER_TIMEOUT_US,
    .f_electrical_zero_deg = FOC_M2_ELECTRICAL_ZERO_DEG,
    .f_control_period_s = FOC_M2_CONTROL_PERIOD_S,
    .f_pwm_period_s = FOC_M2_PWM_PERIOD_S,
    .st_d_pi_cfg =
    {
        .f_kp = FOC_M2_D_KP,
        .f_ki = FOC_M2_D_KI,
        .f_out_max = FOC_M2_D_OUT_MAX_V,
        .f_int_max = FOC_M2_D_INT_MAX_V,
    },
    .st_q_pi_cfg =
    {
        .f_kp = FOC_M2_Q_KP,
        .f_ki = FOC_M2_Q_KI,
        .f_out_max = FOC_M2_Q_OUT_MAX_V,
        .f_int_max = FOC_M2_Q_INT_MAX_V,
    },
    .f_phase_overcurrent_a = FOC_M2_PHASE_OVERCURRENT_A,
    .st_speed_cfg =
    {
        .f_kp = FOC_M2_SPEED_KP,
        .f_ki = FOC_M2_SPEED_KI,
        .f_out_max = FOC_M2_SPEED_OUT_LIMIT_A,
        .f_int_max = FOC_M2_SPEED_OUT_LIMIT_A,
    },
    .st_position_cfg =
    {
        .f_kp = FOC_M2_POSITION_KP,
        .f_ki = FOC_M2_POSITION_KI,
        .f_kd = FOC_M2_POSITION_KD,
        .f_out_max = FOC_M2_POSITION_OUT_LIMIT_RAD_S,
        .f_int_max = FOC_M2_POSITION_OUT_LIMIT_RAD_S,
        .f_d_rc = 0.0f,
    },
    .u32_speed_div = 4U,
    .u32_position_div = 20U
};
//****************************** Constants end *******************************//

//************************ Function Definitions begin ************************//

/**
 * @brief 立即关断两台驱动，初始化电机状态及电机 1 采样链。
 *
 * Steps:
 *  1. 立即关断 EN 及 PWM，不依赖编码器或标定状态。
 *  2. 分别初始化两台电机软件状态，加载各自的 Flash 零偏。
 *  3. 建立电机 1 采样链并注册控制回调。
 *  4. 外环配置完整后尝试开启，否则保持待机并允许按键标定。
 *
 * @return app_status_t : 初始化结果。
 * @note HRTIM 须事先完成硬件初始化，ADC 须事先完成内部校准。
 */
app_status_t service_foc_init(void)
{
    st_service_foc_motor_t *p_motor;
    app_status_t status;

    /* 尚无编码器反馈，先立即关断两台驱动及全部 PWM。 */
    status = service_foc_motor1_pwm_enable(0U);
    if (CORE_GPIO_OK != core_gpio_write_pin(CORE_GPIO_MOTOR2_EN,
            CORE_GPIO_PIN_RESET))
    {
        status = APP_STATUS_IO_ERROR;
    }
    if (HAL_OK != HAL_HRTIM_WaveformOutputStop(&hhrtim1,
            HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TE1 | HRTIM_OUTPUT_TF1))
    {
        status = APP_STATUS_IO_ERROR;
    }
    if (APP_STATUS_OK != status) { return status; }
    /* 初始化期间不执行控制周期。 */
    if (CORE_ADC_OK !=
        core_adc_register_injected_callback(CORE_ADC1, NULL))
    {
        return APP_STATUS_IO_ERROR;
    }
    status = service_foc_motor1_get_state(&p_motor);
    if (APP_STATUS_OK != status) { return status; }
    status = service_foc_software_init(p_motor, &g_st_motor1_config);
    if (APP_STATUS_OK != status)
    {
        return status;
    }
    // status = service_foc_motor2_get_state(&p_motor);
    // if (APP_STATUS_OK != status) { return status; }
    // status = service_foc_software_init(p_motor, &g_st_motor2_config);
    // if (APP_STATUS_OK != status)
    // {
    //     return status;
    // }

    status = service_foc_hardware_init();
    if (APP_STATUS_OK != status)
    {
        return status;
    }
    core_delay_us(FOC_M1_ENCODER_TIMEOUT_US);
    /* 待整定配置不能阻止 App 完成初始化，否则按键标定无法运行。
     * 显式调用 pwm_enable(1) 仍会拒绝未初始化的外环。 */
    if (!p_motor->st_control.st_speed_loop.b_init ||
        !p_motor->st_control.st_position_loop.b_init ||
        (0U == p_motor->st_status.u8_electrical_zero_valid))
    {
        return APP_STATUS_OK;
    }
    /* 有效标定记录已加载，初始化完成后直接开启位置保持。 */
    return service_foc_motor1_pwm_enable(1U);
}
/**
 * @brief 根据传入的电机状态及配置初始化单台电机的软件。
 *
 * Steps:
 *  1. 检查电机状态和配置，加载该电机的 Flash 电角度零偏。
 *  2. 清空并填充该电机的软件状态及控制器参数。
 *  3. 设置电压限幅并初始化该电机的电流环 PI。
 *
 * @param[in,out] p_motor : 待初始化的单台电机状态。
 * @param[in] p_config : 该电机的只读配置。
 * @return app_status_t : 参数、Flash 读取或控制器初始化结果。
 * @note 调用前须关闭输出并解除控制回调，不操作硬件采样链。
 */
app_status_t service_foc_software_init(
    st_service_foc_motor_t *p_motor,
    const st_service_foc_motor_config_t *p_config)
{
    st_speed_loop_cfg_t st_speed_cfg;
    st_position_loop_cfg_t st_position_cfg;
    float f_zero_deg;
    float f_v_limit;
    int8_t i8_direction;
    uint8_t u8_zero_valid = 0U;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_motor) || (NULL == p_config) ||
        ((1U == p_config->u8_motor_id) &&
         ((FOC_M1_SPEED_DIV != p_config->u32_speed_div) ||
          (FOC_M1_POSITION_DIV != p_config->u32_position_div))) ||
        (0U == p_config->u32_speed_div) ||
        (0U == p_config->u32_position_div) ||
        !(p_config->f_control_period_s > 0.0f) ||
        !isfinite(p_config->f_control_period_s) ||
        ((1U != p_config->u8_motor_id) && (2U != p_config->u8_motor_id)) ||
        ((1U == p_config->u8_motor_id) &&
         (CORE_ADC2 != p_config->en_adc_instance)) ||
        ((2U == p_config->u8_motor_id) &&
         (CORE_ADC1 != p_config->en_adc_instance)) ||
        (0U == p_config->u16_pwm_period) ||
        ((-1 != p_config->i8_encoder_direction) &&
         (0 != p_config->i8_encoder_direction) &&
         (1 != p_config->i8_encoder_direction)) ||
        (SERVICE_FOC_OUTPUT_ENABLED == p_motor->st_status.en_output_state) ||
        (SERVICE_FOC_OUTPUT_STARTING == p_motor->st_status.en_output_state) ||
        !(p_config->f_bus_voltage_v > 0.0f) ||
        !isfinite(p_config->f_bus_voltage_v))
    {
        return APP_STATUS_INVALID_PARAM;
    }
    /* 拒绝越界及非有限限幅，避免无效配置进入运行状态。 */
    f_v_limit = p_config->f_bus_voltage_v * 0.57735026919f;
    if (!(p_config->st_d_pi_cfg.f_out_max >= 0.0f &&
          p_config->st_d_pi_cfg.f_out_max <= f_v_limit) ||
        !(p_config->st_q_pi_cfg.f_out_max >= 0.0f &&
          p_config->st_q_pi_cfg.f_out_max <= f_v_limit) ||
        !(p_config->st_d_pi_cfg.f_int_max >= 0.0f &&
          p_config->st_d_pi_cfg.f_int_max <= f_v_limit) ||
        !(p_config->st_q_pi_cfg.f_int_max >= 0.0f &&
          p_config->st_q_pi_cfg.f_int_max <= f_v_limit))
    {
        return APP_STATUS_INVALID_PARAM;
    }
    //--------------------------参数检查 end--------------------------//

    /* 有效标定值优先于默认零偏，不修改只读配置。 */
    f_zero_deg = p_config->f_electrical_zero_deg;
    i8_direction = p_config->i8_encoder_direction;
    if (APP_STATUS_OK != service_foc_zero_load(p_config->u8_motor_id,
            &f_zero_deg, &i8_direction, &u8_zero_valid))
    {
        return APP_STATUS_IO_ERROR;
    }

    memset(p_motor, 0, sizeof(*p_motor));
    p_motor->u8_motor_id = p_config->u8_motor_id;
    p_motor->st_config.en_adc_instance = p_config->en_adc_instance;
    p_motor->st_config.u16_pwm_period = p_config->u16_pwm_period;
    /* 配置不能使软件状态绕过实际硬件开关。 */
    p_motor->st_status.en_output_state = SERVICE_FOC_OUTPUT_STOP_REQUEST;
    p_motor->st_feedback.f_bus_voltage_v = p_config->f_bus_voltage_v;
    /* 母线配置校验后只计算一次，SVPWM 运行路径只乘不除。 */
    p_motor->st_config.f_inv_bus_voltage =
        1.0f / p_config->f_bus_voltage_v;
    p_motor->st_config.u8_pole_pairs = p_config->u8_pole_pairs;
    p_motor->st_config.i8_encoder_direction =
        i8_direction;
    p_motor->st_config.u32_encoder_timeout_us =
        p_config->u32_encoder_timeout_us;
    p_motor->st_config.f_electrical_zero_deg = f_zero_deg;
    p_motor->st_status.u8_electrical_zero_valid = (0U != u8_zero_valid);
    p_motor->st_config.st_d_pi_cfg = p_config->st_d_pi_cfg;
    p_motor->st_config.st_q_pi_cfg = p_config->st_q_pi_cfg;
    p_motor->st_config.st_d_pi_cfg.f_dt = p_config->f_control_period_s;
    p_motor->st_config.st_q_pi_cfg.f_dt = p_config->f_control_period_s;
    p_motor->st_config.f_pwm_period_s = p_config->f_pwm_period_s;
    p_motor->st_config.f_phase_overcurrent_a =
        p_config->f_phase_overcurrent_a;
    st_speed_cfg = p_config->st_speed_cfg;
    st_position_cfg = p_config->st_position_cfg;
    st_speed_cfg.f_dt = p_config->f_control_period_s *
        (float)p_config->u32_speed_div;
    st_position_cfg.f_dt = p_config->f_control_period_s *
        (float)p_config->u32_position_div;
    /* 外环未配置时保持 b_init=false，不阻断采样及零偏标定。 */
    (void)speed_loop_init(&p_motor->st_control.st_speed_loop,
        &st_speed_cfg);
    (void)position_loop_init(&p_motor->st_control.st_position_loop,
        &st_position_cfg);

    /* 轴限幅来自配置；最终电压矢量仍受母线电压约束。 */
    if (CURRENT_LOOP_OK != current_loop_init(
            &p_motor->st_control.st_current_loop,
            &p_motor->st_config.st_d_pi_cfg,
            &p_motor->st_config.st_q_pi_cfg,
            f_v_limit))
    {
        return APP_STATUS_INVALID_PARAM;
    }
    p_motor->st_target.f_id_ref_a = 0.0f;
    p_motor->st_target.f_iq_ref_a = 0.0f;
    p_motor->st_status.u8_control_initialized = 1U;
    return APP_STATUS_OK;
}


/**
 * @brief 建立共享采样链，阻塞停机及校准电流，始终保持待机输出关闭。
 * @return app_status_t : 硬件/采样失败返回错误；未确认阈值只诊断。
 * @note ADC 内部校准由 App 在采样开始前完成；电机 2 不启用。
 */
app_status_t service_foc_hardware_init(void)
{
    st_service_foc_motor_t *p_motor;
    app_status_t status;
    const uint32_t u32_motor_timers = HRTIM_TIMERID_TIMER_A |
        HRTIM_TIMERID_TIMER_B | HRTIM_TIMERID_TIMER_C;

    status = service_foc_motor1_get_state(&p_motor);
    if (APP_STATUS_OK != status) { return status; }
    p_motor->st_status.u8_hardware_ready = 0U;
    status = service_foc_motor1_pwm_enable(0U);
    if (APP_STATUS_OK != status) { return status; }
    if (BSP_WRAPPER_ENCODER_OK != bsp_wrapper_encoder_init())
    {
        return APP_STATUS_IO_ERROR;
    }
    if (CORE_ADC_OK != core_adc_injected_start(CORE_ADC2))
    {
        return APP_STATUS_IO_ERROR;
    }
    if ((CORE_ADC_OK != core_adc_injected_start_it(CORE_ADC1)) ||
        (CORE_TIME_OK != core_hrtim_master_start()) ||
        (HAL_OK != HAL_HRTIM_WaveformCounterStart(&hhrtim1,
            u32_motor_timers)))
    {
        status = APP_STATUS_IO_ERROR;
        goto fail;
    }
    p_motor->st_status.u8_hardware_ready = 1U;
    status = service_foc_motor1_pwm_enable(0U);
    if (APP_STATUS_OK != status)
    {
        goto fail;
    }
    if (CORE_ADC_OK != core_adc_register_injected_callback(
            CORE_ADC1, service_foc_motor1_foc_task))
    {
        status = APP_STATUS_IO_ERROR;
        goto fail;
    }
    return APP_STATUS_OK;
fail:
    p_motor->st_status.u8_hardware_ready = 0U;
    (void)service_foc_motor1_pwm_enable(0U);
    (void)core_adc_register_injected_callback(CORE_ADC1, NULL);
    (void)core_adc_injected_stop_it(CORE_ADC1);
    (void)core_adc_injected_stop_it(CORE_ADC2);
    return status;
}

//************************* Function Definitions end *************************//

