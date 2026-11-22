/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file service_foc_motor1.c
 *
 * @par dependencies
 * - service_foc_motor1.h
 * - ../../04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h
 * - gpio_port.h
 * - hrtim.h
 *
 * @author YA
 *
 * @brief FOC 服务初始化与 ADC 电流控制周期入口。
 *
 * Processing flow:
 *
 * 初始化编码器和电流服务 -> 注册 ADC -> 读取电流和编码器 -> 执行控制周期。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "service_foc_motor1.h"
#include "service_foc_init.h"
#include "service_foc_config.h"
#include <math.h>
#include "service_foc_zero_load/service_foc_zero_load.h"
#include "../../04_platform/platform_bsp/current/wrapper/bsp_wrapper_current.h"
#include "../../04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h"
#include "../../04_platform/platform_mcu/gpio/inc/gpio_port.h"
#include "hrtim.h"
#include "../../03_config/feature_config.h"
#include "../../06_middlewares/algorithm/FOC/inc/foc_transform.h"
#include "../../06_middlewares/algorithm/FOC/inc/sin_lut.h"
#include "../../06_middlewares/debug/dwt_measure.h"
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
#define SERVICE_FOC_DEG_TO_RAD (0.01745329251994329577f)
//******************************* Defines end ********************************//

//************************** Static Variables begin **************************//
static st_service_foc_motor_t g_st_motor1;
//*************************** Static Variables end ***************************//

//************************** Global Variables begin **************************//
//*************************** Global Variables end ***************************//

//************************** Static Functions begin **************************//
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//

/**
 * @brief 以 20kHz 调用，执行 5kHz DMA 及 1kHz 位置环、速度环。
 * Steps:
 *  1. 推进循环拍号，DMA 启动独立于普通输出状态。
 *  2. 1kHz 先由位置环生成速度给定，有测速样本时再计算速度环。
 * @return app_status_t : 分区任务执行状态，关断由 FOC 入口处理。
 * @note 每拍只能由 FOC 入口调用一次；标定期间该回调已暂停。
 */
app_status_t service_foc_motor1_5khz_task(void)
{
    uint32_t u32_failed = 0U;
    bsp_wrapper_encoder_status_t en_velocity_status;
    float f_average_velocity_rad_s;
    float f_position_rad;

    //拍号,0~20循环
    //---------------------------5kHz 区----------------------------//
    /* DMA 保持 5kHz，与 1kHz 速度环独立分频并错峰。 */
    if (FOC_M1_DMA_PHASE ==
        (g_st_motor1.st_control.u32_schedule_tick & (FOC_M1_DMA_DIV - 1U)))
    {
        (void)bsp_wrapper_encoder_start_angle_dma();
    }
    if ((FOC_M1_POSITION_PHASE ==
            g_st_motor1.st_control.u32_schedule_tick) &&
        (SERVICE_FOC_OUTPUT_ENABLED ==
            g_st_motor1.st_status.en_output_state))
    {
        /* 机械角度与速度反馈采用相同方向，单圈归一化。 */
        f_position_rad = fmodf(
            g_st_motor1.st_feedback.f_mechanical_angle_deg *
            (float)g_st_motor1.st_config.i8_encoder_direction *
            SERVICE_FOC_DEG_TO_RAD, POSITION_LOOP_TWO_PI);
        if (f_position_rad < 0.0f)
        {
            f_position_rad += POSITION_LOOP_TWO_PI;
        }
        u32_failed |= (POSITION_LOOP_OK != position_loop_calc(
            &g_st_motor1.st_control.st_position_loop, f_position_rad,
            &g_st_motor1.st_control.f_speed_ref_rad_s));
    }
    if (FOC_M1_SPEED_PHASE ==
        g_st_motor1.st_control.u32_schedule_tick)
    {
#if (CFG_FEATURE_MEASURE_FOC_SPEED_LOOP > 0U)
        (void)dwt_measure_enter();
#endif
        en_velocity_status = bsp_wrapper_encoder_get_angular_velocity(
            &f_average_velocity_rad_s);
        if ((0U == u32_failed) &&
            (BSP_WRAPPER_ENCODER_OK == en_velocity_status) &&
            (SERVICE_FOC_OUTPUT_ENABLED ==
                g_st_motor1.st_status.en_output_state))
        {
            u32_failed = (SPEED_LOOP_OK != speed_loop_calc(
                &g_st_motor1.st_control.st_speed_loop,
                g_st_motor1.st_control.f_speed_ref_rad_s,
                f_average_velocity_rad_s *
                    (float)g_st_motor1.st_config.i8_encoder_direction,
                &g_st_motor1.st_target.f_iq_ref_a));
        }
        else if ((BSP_WRAPPER_ENCODER_OK != en_velocity_status) &&
                 (BSP_WRAPPER_ENCODER_ERR_EMPTY != en_velocity_status))
        {
            u32_failed = 1U;
        }
#if (CFG_FEATURE_MEASURE_FOC_SPEED_LOOP > 0U)
        (void)dwt_measure_exit();
#endif
    }

    g_st_motor1.st_control.u32_schedule_tick++;
    if (FOC_M1_POSITION_DIV <= g_st_motor1.st_control.u32_schedule_tick)
    {
        g_st_motor1.st_control.u32_schedule_tick = 0U;
    }
    return (0U == u32_failed) ? APP_STATUS_OK : APP_STATUS_IO_ERROR;
}
/**
 * @brief 响应 ADC 完成事件，执行电机 1 的完整 20kHz FOC 周期。
 *
 * Steps:
 *  1. 更新编码器反馈并读取三相电流。
 *  2. 固定分频错峰执行外环，再计算电流环。
 *  3. 仅完整计算成功时写入 PWM，末尾统一根据 status 处理故障。
 *
 * @param[in] instance : ADC 逻辑实例，电机 1 当前由 ADC1 回调触发。
 * @return void : ADC 回调固定签名，无返回值。
 */
void service_foc_motor1_foc_task(en_core_adc_instance_t instance)
{
//注册的回调函数必须要参数,但是这里不用,因为要快速没有去跟普通的adc回调一起
(void)instance; 
#if (CFG_FEATURE_MEASURE_ISR_LATENCY > 0U)
/* 计时终点：中断入口 -> 走到本行（开始检测前）的耗时。 */
(void)dwt_measure_exit();
#endif

    uint32_t status = 0U; /* 累计失败结果，0 表示本周期全部成功。 */

#if (CFG_FEATURE_MEASURE_ENCODER_TASK_TIME > 0U)
    (void)dwt_measure_enter();
#endif
    service_foc_motor1_encoder_task();        //编码器,去DMA搬运的缓冲区读最新角度
#if (CFG_FEATURE_MEASURE_ENCODER_TASK_TIME > 0U)
    (void)dwt_measure_exit();
#endif
    
#if (CFG_FEATURE_MEASURE_CURRENT_TASK_TIME > 0U)
    (void)dwt_measure_enter();
#endif
    service_foc_motor1_current_task();
#if (CFG_FEATURE_MEASURE_CURRENT_TASK_TIME > 0U)
    (void)dwt_measure_exit();
#endif

#if (CFG_FEATURE_MEASURE_5KHZ_TASK_TIME > 0U)
    (void)dwt_measure_enter();
#endif
    status |= (uint32_t)service_foc_motor1_5khz_task();
#if (CFG_FEATURE_MEASURE_5KHZ_TASK_TIME > 0U)
    (void)dwt_measure_exit();
#endif

    if (0U == status)
    {
        service_foc_motor1_current_loop_task();
    }
    status |= (uint32_t)(SERVICE_FOC_OUTPUT_ENABLED !=
        g_st_motor1.st_status.en_output_state);

    /* 最后统一判断；存在失败时短路跳过 PWM 提交，直接关断。 */
    if ((0U != status) ||
        (CORE_TIME_OK != core_hrtim_set_motor_cmp(1U,
            g_st_motor1.st_phase.u16_pwm_cmp_a,
            g_st_motor1.st_phase.u16_pwm_cmp_b,
            g_st_motor1.st_phase.u16_pwm_cmp_c)))
    {
        (void)service_foc_motor1_pwm_enable(0U);
    }
#if (CFG_FEATURE_MEASURE_ISR_TOTAL > 0U)
    (void)dwt_measure_exit();
#endif
    return;
}


/**
 * @brief 更新电机 1 机械反馈并计算电角度。
 *
 * Steps:
 *  1. 更新机械反馈，同时检查样本是否存在且未过期。
 *  2. 反馈无效时关断输出并返回。
 *  3. 每拍计算含零偏的电角度，归一化到 [0, 360) 度并保存。
 *
 * @return void : 无返回值。
 * @note 极对数和方向由初始化保证；每拍转换 wrapper 外推的机械角度。
 */
void service_foc_motor1_encoder_task(void)
{
    int32_t i32_scale;

    /* 共用样本年龄完成超时检查和角度外推。 */
    g_st_motor1.st_status.u8_encoder_valid =
        (BSP_WRAPPER_ENCODER_OK == bsp_wrapper_encoder_update(
        g_st_motor1.st_config.u32_encoder_timeout_us,
        &g_st_motor1.st_feedback.f_mechanical_angle_deg,
        &g_st_motor1.st_feedback.f_mechanical_velocity_rad_s));

    /* 先处理反馈超时，避免无新样本时提前返回漏掉关断。 */
    if (0U == g_st_motor1.st_status.u8_encoder_valid)
    {
        (void)service_foc_motor1_pwm_enable(0U);
        return;
    }

    /* 无新 DMA 样本时 wrapper 仍会按角速度外推，必须每拍转换。 */

    /* 先以整数合并极对数和方向，只转换一次并做一次浮点乘法。 */
    i32_scale = (int32_t)g_st_motor1.st_config.u8_pole_pairs *
        (int32_t)g_st_motor1.st_config.i8_encoder_direction;
    /* 电角度 = 机械角度 × 极对数 × 编码器方向 + 电角度零偏。 */
    float f_elec = (g_st_motor1.st_feedback.f_mechanical_angle_deg *
        (float)i32_scale) +
        g_st_motor1.st_config.f_electrical_zero_deg;

    /* 将正向超周和反向角度统一折算到 [0, 360) 度。 */
    while (f_elec >= 360.0f)
    {
        f_elec -= 360.0f;
    }
    while (f_elec < 0.0f)
    {
        f_elec += 360.0f;
    }

    /* 保存本次电角度，供后续坐标变换使用。 */
    g_st_motor1.st_feedback.f_electrical_angle_deg = f_elec;
    g_st_motor1.st_status.u8_angle_update_pending = 0U;

}

/**
 * @brief 读取电机 1 电流，选取两相并重构下管窗口最短的一相。
 *
 * Steps:
 *  1. 从 ADC2 读取电流，读取失败时关断输出并返回。
 *  2. 按本次采样对应的 PWM 比较值选择两相，重构第三相。
 *  3. 检查两相实测电流及重构电流是否过流。
 *
 * @return void : 无返回值。
 * @note 每个 20kHz 控制周期调用，先读取电流，再执行电流环计算。
 */
void service_foc_motor1_current_task(void)
{
    bsp_wrapper_current_status_t status;

#if (CFG_FEATURE_MEASURE_ADC_TIME > 0U)
    (void)dwt_measure_enter();
#endif
    status = bsp_wrapper_current_read(CORE_ADC2,
        &g_st_motor1.st_phase.f_current_a,
        &g_st_motor1.st_phase.f_current_b,
        &g_st_motor1.st_phase.f_current_c);
#if (CFG_FEATURE_MEASURE_ADC_TIME > 0U)
    (void)dwt_measure_exit();
#endif

    /* 当前 PWM 模式下，比较值最小的相下管窗口最短。
     * 等价于扇区 1/6 重构 A、2/3 重构 B、4/5 重构 C。
     * 此处尚未计算下一周期 PWM，使用本次采样对应的比较值；
     * 两次整数比较选相，只需一次浮点加法和取负。 */
    if (g_st_motor1.st_phase.u16_pwm_cmp_a <=
        g_st_motor1.st_phase.u16_pwm_cmp_b)
    {
        if (g_st_motor1.st_phase.u16_pwm_cmp_a <=
            g_st_motor1.st_phase.u16_pwm_cmp_c)
        {
            g_st_motor1.st_phase.f_current_a =
                -(g_st_motor1.st_phase.f_current_b +
                  g_st_motor1.st_phase.f_current_c);
        }
        else
        {
            g_st_motor1.st_phase.f_current_c =
                -(g_st_motor1.st_phase.f_current_a +
                  g_st_motor1.st_phase.f_current_b);
        }
    }
    else
    {
        if (g_st_motor1.st_phase.u16_pwm_cmp_b <=
            g_st_motor1.st_phase.u16_pwm_cmp_c)
        {
            g_st_motor1.st_phase.f_current_b =
                -(g_st_motor1.st_phase.f_current_a +
                  g_st_motor1.st_phase.f_current_c);
        }
        else
        {
            g_st_motor1.st_phase.f_current_c =
                -(g_st_motor1.st_phase.f_current_a +
                  g_st_motor1.st_phase.f_current_b);
        }
    }

    /* 检查实测及重构电流，保护阈值独立于目标电流上限。 */
    if (!(fabsf(g_st_motor1.st_phase.f_current_a) <=
            g_st_motor1.st_config.f_phase_overcurrent_a) ||
        !(fabsf(g_st_motor1.st_phase.f_current_b) <=
            g_st_motor1.st_config.f_phase_overcurrent_a) ||
        !(fabsf(g_st_motor1.st_phase.f_current_c) <=
            g_st_motor1.st_config.f_phase_overcurrent_a) ||
        BSP_WRAPPER_CURRENT_OK != status)
    {
        (void)service_foc_motor1_pwm_enable(0U);
    }
}

/**
 * @brief 执行电机 1 电流环并计算三相 PWM 比较值。
 *
 * Steps:
 *  1. 使用已读取的三相电流，经 Clarke 和 Park 变换得到 d/q 轴电流。
 *  2. 执行电流环，生成 d/q 轴电压并进行逆 Park 变换。
 *  3. 执行 SVPWM，保存三相比较值。
 *
 * @return void : 无返回值。
 * @note 初始化由开启入口检查，反馈有效性由编码器任务负责。
 *       电流环或 SVPWM 失败时直接关闭输出并返回。
 *       每个 20kHz 控制周期在电流读取后调用，比较值由周期入口写入硬件。
 */
void service_foc_motor1_current_loop_task(void)
{
    svpwm_input_t st_pwm_in;
    svpwm_output_t st_pwm_out;

    /* Clarke 变换：将 A/B/C 三相电流转换为静止坐标系的 alpha/beta 电流。 */
#if (CFG_FEATURE_MEASURE_FOC_CLARKE > 0U)
    (void)dwt_measure_enter();
#endif
    clarke_transform_abc(
        g_st_motor1.st_phase.f_current_a,     /* A 相电流/A */
        g_st_motor1.st_phase.f_current_b,     /* B 相电流/A */
        g_st_motor1.st_phase.f_current_c,     /* C 相电流/A */
        &g_st_motor1.st_control.f_i_alpha_a,  /* alpha 电流输出/A */
        &g_st_motor1.st_control.f_i_beta_a);  /* beta 电流输出/A */
#if (CFG_FEATURE_MEASURE_FOC_CLARKE > 0U)
    (void)dwt_measure_exit();
#endif

    /* 将电角度由度转换为弧度，查表获取正弦和余弦值。 */
#if (CFG_FEATURE_MEASURE_FOC_SIN_LUT > 0U)
    (void)dwt_measure_enter();
#endif
    sin_lut_sincos(
        g_st_motor1.st_feedback.f_electrical_angle_deg *
        SERVICE_FOC_DEG_TO_RAD,                     /* 电角度/rad */
        &g_st_motor1.st_control.f_sin_electrical,   /* 正弦输出 */
        &g_st_motor1.st_control.f_cos_electrical);  /* 余弦输出 */
#if (CFG_FEATURE_MEASURE_FOC_SIN_LUT > 0U)
    (void)dwt_measure_exit();
#endif

    /* Park 变换：利用电角度正余弦，将 alpha/beta 电流转换为 d/q 电流。 */
#if (CFG_FEATURE_MEASURE_FOC_PARK > 0U)
    (void)dwt_measure_enter();
#endif
    park_transform_sc(
        g_st_motor1.st_control.f_i_alpha_a,       /* alpha 电流/A */
        g_st_motor1.st_control.f_i_beta_a,        /* beta 电流/A */
        g_st_motor1.st_control.f_sin_electrical,  /* 电角度正弦 */
        g_st_motor1.st_control.f_cos_electrical,  /* 电角度余弦 */
        &g_st_motor1.st_control.f_id_a,           /* d 轴电流输出/A */
        &g_st_motor1.st_control.f_iq_a);          /* q 轴电流输出/A */
#if (CFG_FEATURE_MEASURE_FOC_PARK > 0U)
    (void)dwt_measure_exit();
#endif

    /* 根据目标电流与反馈电流计算 d/q 轴电压。 */
#if (CFG_FEATURE_MEASURE_FOC_CURRENT_LOOP > 0U)
    (void)dwt_measure_enter();
#endif
    if (CURRENT_LOOP_OK != current_loop_calc(
            &g_st_motor1.st_control.st_current_loop,  /* 电流环对象 */
            g_st_motor1.st_target.f_id_ref_a,         /* d 轴目标电流/A */
            g_st_motor1.st_target.f_iq_ref_a,         /* q 轴目标电流/A */
            g_st_motor1.st_control.f_id_a,            /* d 轴反馈电流/A */
            g_st_motor1.st_control.f_iq_a,            /* q 轴反馈电流/A */
            &g_st_motor1.st_control.f_vd_v,           /* d 轴电压输出/V */
            &g_st_motor1.st_control.f_vq_v))          /* q 轴电压输出/V */
    {
#if (CFG_FEATURE_MEASURE_FOC_CURRENT_LOOP > 0U)
        (void)dwt_measure_exit();
#endif
        (void)service_foc_motor1_pwm_enable(0U);
        return;
    }

#if (CFG_FEATURE_MEASURE_FOC_CURRENT_LOOP > 0U)
    (void)dwt_measure_exit();
#endif

    /* 逆 Park 变换：利用电角度正余弦，将 d/q 电压转换为 alpha/beta 电压。 */
#if (CFG_FEATURE_MEASURE_FOC_INVERSE_PARK > 0U)
    (void)dwt_measure_enter();
#endif
    inverse_park_sc(
        g_st_motor1.st_control.f_vd_v,            /* d 轴电压/V */
        g_st_motor1.st_control.f_vq_v,            /* q 轴电压/V */
        g_st_motor1.st_control.f_sin_electrical,  /* 电角度正弦 */
        g_st_motor1.st_control.f_cos_electrical,  /* 电角度余弦 */
        &g_st_motor1.st_control.f_v_alpha_v,      /* alpha 电压输出/V */
        &g_st_motor1.st_control.f_v_beta_v);      /* beta 电压输出/V */
#if (CFG_FEATURE_MEASURE_FOC_INVERSE_PARK > 0U)
    (void)dwt_measure_exit();
#endif

    st_pwm_in.u_alpha = g_st_motor1.st_control.f_v_alpha_v;/* alpha 电压/V */
    st_pwm_in.u_beta = g_st_motor1.st_control.f_v_beta_v;  /* beta 电压/V */
    st_pwm_in.inv_v_dc = g_st_motor1.st_config.f_inv_bus_voltage;
    st_pwm_in.t_pwm = g_st_motor1.st_config.f_pwm_period_s;/* PWM 周期/s */
    st_pwm_in.k0 = 0.5f;                        /* 零矢量系数，七段式 */
    st_pwm_in.arr = g_st_motor1.st_config.u16_pwm_period; /* HRTIM 周期计数 */

    /* 计算并保存 U/V/W 三相比较计数 ccr_u/v/w，对应电机 A/B/C 相；
     * 失败时直接关闭输出并返回，成功时保存结果。 */
#if (CFG_FEATURE_MEASURE_FOC_SVPWM > 0U)
    (void)dwt_measure_enter();
#endif
    if (SVPWM_OK != svpwm_calc(&st_pwm_in, &st_pwm_out))
    {
#if (CFG_FEATURE_MEASURE_FOC_SVPWM > 0U)
        (void)dwt_measure_exit();
#endif
        (void)service_foc_motor1_pwm_enable(0U);
        return;
    }
#if (CFG_FEATURE_MEASURE_FOC_SVPWM > 0U)
    (void)dwt_measure_exit();
#endif

    /* 保存 SVPWM 比较计数：U/V/W 分别对应 A/B/C 相，由 FOC 入口写入硬件。 */
    g_st_motor1.st_phase.u16_pwm_cmp_a = st_pwm_out.ccr_u;
    g_st_motor1.st_phase.u16_pwm_cmp_b = st_pwm_out.ccr_v;
    g_st_motor1.st_phase.u16_pwm_cmp_c = st_pwm_out.ccr_w;
}

/**
 * @brief 切换电机 1 输出，普通开启前计算一轮控制结果。
 *
 * Steps:
 *  1. 重复开关直接返回，首次关闭仍执行硬件关断。
 *  2. 关闭时关断输出并清 PWM 比较值。
 *  3. 普通开启时清 PI、计算比较值；标定沿用预设比较值。
 *
 * @param[in] u8_enable : 0 关闭，1 开启。
 * @return app_status_t : 参数、就绪或硬件操作结果。
 * @note 不阻塞读取编码器或等待停稳，使用已有反馈完成计算。
 */
app_status_t service_foc_motor1_pwm_enable(uint8_t u8_enable)
{
    app_status_t status = APP_STATUS_OK;
    uint8_t u8_calibrating = 0;
    uint32_t u32_mask;
    float f_position_rad;

    //-------------------------参数检查 begin-------------------------//
    if (1 < u8_enable) { return APP_STATUS_INVALID_PARAM; }
    u32_mask = __get_PRIMASK();
    __disable_irq();
    if (((1 == u8_enable) && (SERVICE_FOC_OUTPUT_ENABLED ==
             g_st_motor1.st_status.en_output_state)) ||
        ((0 == u8_enable) &&
         ((SERVICE_FOC_OUTPUT_STOPPING ==
               g_st_motor1.st_status.en_output_state) ||
          (SERVICE_FOC_OUTPUT_STOPPED ==
               g_st_motor1.st_status.en_output_state))))
    {//1到1或者0到0,直接return ok
        __set_PRIMASK(u32_mask);
        return APP_STATUS_OK;
    }

    if (1 == u8_enable)
    {
        service_foc_zero_get_calibrating(&u8_calibrating);
        if ((SERVICE_FOC_OUTPUT_STARTING ==
                 g_st_motor1.st_status.en_output_state) ||
            (SERVICE_FOC_OUTPUT_STOP_REQUEST ==
                 g_st_motor1.st_status.en_output_state) ||
            (0 == g_st_motor1.st_status.u8_hardware_ready) ||
            ((0 == u8_calibrating) &&
             ((0 == g_st_motor1.st_status.u8_control_initialized) ||
              !g_st_motor1.st_control.st_speed_loop.b_init ||
              !g_st_motor1.st_control.st_position_loop.b_init)))
        {//正在预计算，尚未准备完成
         //尚未确认硬件已成功关断
         //硬件采样链尚未就绪
         //非标定模式下，电流 PI 尚未初始化
            __set_PRIMASK(u32_mask);
            return APP_STATUS_INVALID_PARAM;//不让开启
        }
        /* 未标定独立返回，应用可保持待机并处理标定按键。 */
        if ((0 == u8_calibrating) &&
            (0 == g_st_motor1.st_status.u8_electrical_zero_valid))
        {
            __set_PRIMASK(u32_mask);
            return APP_STATUS_NOT_CALIBRATED;
        }
    //--------------------------参数检查 end--------------------------//
        if (0 == u8_calibrating)
        {
            /* 使用现有反馈，开启前清 PI 并计算一次，不等待 IIC。 */
            g_st_motor1.st_status.en_output_state = SERVICE_FOC_OUTPUT_STARTING;
            service_foc_motor1_encoder_task();
            service_foc_motor1_current_task();
            if (SERVICE_FOC_OUTPUT_STARTING !=
                    g_st_motor1.st_status.en_output_state)
            {
                __set_PRIMASK(u32_mask);
                return APP_STATUS_IO_ERROR;
            }
            /* 开启前清 PI，拍号归零，速度环在第19拍读取新窗口。 */
            g_st_motor1.st_control.u32_schedule_tick = 0U;
            /* 检查已标定的编码器方向并清除三环历史。 */
            if (((1 != g_st_motor1.st_config.i8_encoder_direction) &&
                 (-1 != g_st_motor1.st_config.i8_encoder_direction)) ||
                (POSITION_LOOP_OK != position_loop_reset(
                    &g_st_motor1.st_control.st_position_loop)) ||
                (SPEED_LOOP_OK != speed_loop_reset(
                    &g_st_motor1.st_control.st_speed_loop)) ||
                (CURRENT_LOOP_OK != current_loop_reset(
                    &g_st_motor1.st_control.st_current_loop)))
            {
                (void)service_foc_motor1_pwm_enable(0U);
                __set_PRIMASK(u32_mask);
                return APP_STATUS_INVALID_PARAM;
            }
            g_st_motor1.st_target.f_id_ref_a = 0.0f;
            /* 未显式设置目标时，每次启动锁定本次位置。 */
            if (!g_st_motor1.st_control.st_position_loop.b_target_valid)
            {
                f_position_rad = fmodf(
                    g_st_motor1.st_feedback.f_mechanical_angle_deg *
                    (float)g_st_motor1.st_config.i8_encoder_direction *
                    SERVICE_FOC_DEG_TO_RAD, POSITION_LOOP_TWO_PI);
                if (f_position_rad < 0.0f)
                {
                    f_position_rad += POSITION_LOOP_TWO_PI;
                }
                g_st_motor1.st_control.st_position_loop.f_position_ref_rad =
                    f_position_rad;
            }
            g_st_motor1.st_control.f_speed_ref_rad_s = 0.0f;
            bsp_wrapper_encoder_reset_feedback_history();
            /* 启动先用零Iq，有新平均速度后由1kHz速度环接管。 */
            g_st_motor1.st_target.f_iq_ref_a = 0.0f;
            service_foc_motor1_current_loop_task();
            if ((SERVICE_FOC_OUTPUT_STARTING !=
                     g_st_motor1.st_status.en_output_state) ||
                (CORE_TIME_OK != core_hrtim_set_motor_cmp(
                     1U,                                    /* 电机编号 */
                     g_st_motor1.st_phase.u16_pwm_cmp_a,     /* A 相比较值 */
                     g_st_motor1.st_phase.u16_pwm_cmp_b,     /* B 相比较值 */
                     g_st_motor1.st_phase.u16_pwm_cmp_c)))   /* C 相比较值 */
            {
                (void)service_foc_motor1_pwm_enable(0U);
                __set_PRIMASK(u32_mask);
                return APP_STATUS_IO_ERROR;
            }
        }
        if ((HAL_OK == HAL_HRTIM_WaveformOutputStart(&hhrtim1,
                 HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TC1)) &&
            (CORE_GPIO_OK == core_gpio_write_pin(CORE_GPIO_MOTOR1_EN,
                 CORE_GPIO_PIN_SET)))//这里开启
        {//开启成功进↓
            g_st_motor1.st_status.en_output_state = SERVICE_FOC_OUTPUT_ENABLED;
            if (1 == u8_calibrating)
            {//如果正在标定中,那么也什么都不能做
                __set_PRIMASK(u32_mask);
                return APP_STATUS_OK;
            }
            __set_PRIMASK(u32_mask);
            return APP_STATUS_OK;//这里开启就返回了
        }
        status = APP_STATUS_IO_ERROR;
    }

    /* 停机时清速度给定、电流指令及外环历史，保留显式位置目标。 */
    bsp_wrapper_encoder_reset_feedback_history();
    g_st_motor1.st_control.f_speed_ref_rad_s = 0.0f;
    g_st_motor1.st_target.f_id_ref_a = 0.0f;
    g_st_motor1.st_target.f_iq_ref_a = 0.0f;
    g_st_motor1.st_control.u32_schedule_tick = 0U;
    (void)speed_loop_reset(&g_st_motor1.st_control.st_speed_loop);
    (void)position_loop_reset(&g_st_motor1.st_control.st_position_loop);

    /* 开启失败也走关断；STARTING 状态下关闭会取消预计算。 */
    g_st_motor1.st_status.en_output_state = SERVICE_FOC_OUTPUT_STOP_REQUEST;
    if (CORE_GPIO_OK != core_gpio_write_pin(CORE_GPIO_MOTOR1_EN,
            CORE_GPIO_PIN_RESET))
    {
        status = APP_STATUS_IO_ERROR;
    }
    if (HAL_OK != HAL_HRTIM_WaveformOutputStop(&hhrtim1,
            HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TC1))
    {
        status = APP_STATUS_IO_ERROR;
    }
    g_st_motor1.st_phase.u16_pwm_cmp_a = 0U;
    g_st_motor1.st_phase.u16_pwm_cmp_b = 0U;
    g_st_motor1.st_phase.u16_pwm_cmp_c = 0U;
    if (CORE_TIME_OK != core_hrtim_set_motor_cmp(1U, 0U, 0U, 0U))
    {
        status = APP_STATUS_IO_ERROR;
    }
    g_st_motor1.st_control.f_vd_v = 0.0f;
    g_st_motor1.st_control.f_vq_v = 0.0f;
    g_st_motor1.st_control.f_v_alpha_v = 0.0f;
    g_st_motor1.st_control.f_v_beta_v = 0.0f;
    g_st_motor1.st_status.u8_control_valid = 0U;
    if (APP_STATUS_OK == status)
    {
        g_st_motor1.st_status.en_output_state = SERVICE_FOC_OUTPUT_STOPPING;
    }
    __set_PRIMASK(u32_mask);
    return status;
}

/**
 * @brief 向初始化和标定模块提供本电机私有状态的地址。
 *
 * Steps:
 *  1. 检查输出指针并返回私有状态地址。
 *
 * @param[out] pp_motor : 电机状态地址的输出位置。
 * @return app_status_t : 成功或参数错误。
 * @note 获取地址不改变状态。位置目标设置用短临界区保护，
 *       初始化、标定或修改其他配置时须暂停本电机控制周期。
 */
app_status_t service_foc_motor1_get_state(st_service_foc_motor_t **pp_motor)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == pp_motor)
    {
        return APP_STATUS_INVALID_PARAM;
    }
    //--------------------------参数检查 end--------------------------//

    *pp_motor = &g_st_motor1;
    return APP_STATUS_OK;
}

//************************* Function Definitions end *************************//
