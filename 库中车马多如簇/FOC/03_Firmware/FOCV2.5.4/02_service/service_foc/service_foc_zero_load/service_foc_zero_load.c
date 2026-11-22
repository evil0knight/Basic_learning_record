/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file service_foc_zero_load.c
 *
 * @par dependencies
 * - service_foc_zero_load.h
 * - ../service_foc_motor1.h
 * - ../../../01_app/app_message/msg_queue.h
 * - ../../../04_platform/platform_bsp/flash/wrapper/bsp_wrapper_flash.h
 * - ../../../04_platform/platform_bsp/current/wrapper/bsp_wrapper_current.h
 * - ../../../04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h
 * - ../../../04_platform/platform_mcu/time/inc/time_port.h
 * - ../../../04_platform/platform_mcu/gpio/inc/gpio_port.h
 * - ../../../06_middlewares/algorithm/FOC/inc/foc_transform.h
 * - hrtim.h
 * - i2c.h
 * - math.h
 * - stdio.h
 * - string.h
 *
 * @author YA
 *
 * @brief 电角度零点标定与零偏存取接口。
 *
 * Processing flow:
 *
 * 加载零偏 -> 主循环对齐与验证 -> 关闭输出 -> 保存零偏。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "service_foc_zero_load.h"
#include "../service_foc_motor1.h"
#include "../service_foc_init.h"
#include "../service_foc_config.h"
#include "../../../01_app/app_message/msg_queue.h"
#include "../../../04_platform/platform_bsp/flash/wrapper/bsp_wrapper_flash.h"
#include "../../../04_platform/platform_bsp/current/wrapper/bsp_wrapper_current.h"
#include "../../../04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h"
#include "../../../04_platform/platform_mcu/time/inc/time_port.h"
#include "../../../04_platform/platform_mcu/gpio/inc/gpio_port.h"
#include "../../../06_middlewares/algorithm/FOC/inc/foc_transform.h"
#include "hrtim.h"
#include "i2c.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
#define SERVICE_FOC_DEG_TO_RAD (0.01745329251994329577f)
/* 版本 2：标识高位区分方向，记录仍为 16 字节并保留双反码校验。 */
#define SERVICE_FOC_ZERO_TAG_FORWARD (0x464F4302UL)
#define SERVICE_FOC_ZERO_TAG_REVERSE (0x464F4382UL)
#define SERVICE_FOC_CURRENT_TAG      (0x43555202UL)
#define SERVICE_FOC_CURRENT_ADDR_OFF (0x00000020UL)
//******************************* Defines end ********************************//

//************************** Static Variables begin **************************//
/* 标定模式标志；整个标定期间解除正常 FOC 回调。 */
static volatile uint8_t g_u8_motor1_calibrating = 0U;
//*************************** Static Variables end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 输出关闭时阻塞读取反馈，连续确认静止并等待电流衰减。
 *
 * Steps:
 *  1. 暂停控制回调，等待在途 DMA 后同步读取编码器。
 *  2. 连续检查角度变化；已有电流零偏时同时检查电流有效及不过流。
 *  3. 静止持续达到稳定和衰减时间后，标记确认停止。
 *
 * @return app_status_t : 成功、参数错误或读取/停稳超时错误。
 * @note 仅主循环调用；未建立电流零偏时跳过换算电流检查。
 *       电流衰减按等待时间判断，不代表已经测得零电流。
 */
app_status_t service_foc_motor1_wait_stop(void)
{
    st_service_foc_motor_t *p_motor;
    app_status_t status;
    bsp_wrapper_current_status_t en_current_status;
    uint32_t u32_start;
    uint32_t u32_stable_start;
    uint32_t u32_previous_us;
    uint32_t u32_now;
    uint32_t u32_elapsed;
    uint32_t u32_mask;
    uint32_t u32_required_ms =
        FOC_M1_STOP_STABLE_MS + FOC_M1_STOP_DECAY_MS;
    uint16_t u16_previous;
    uint16_t u16_raw;
    int32_t i32_delta;
    int32_t i32_position = 0;
    int32_t i32_min = 0;
    int32_t i32_max = 0;
    float f_speed;

    //-------------------------参数检查 begin-------------------------//
    if ((0U != __get_IPSR()) || (0U != __get_PRIMASK()) ||
        (0U != __get_BASEPRI()))
    {
        printf("[CAL] invalid call context\r\n");
        return APP_STATUS_INVALID_PARAM;
    }
    status = service_foc_motor1_get_state(&p_motor);
    if (APP_STATUS_OK != status) { return status; }
    if (((SERVICE_FOC_OUTPUT_STOPPING != p_motor->st_status.en_output_state) &&
         (SERVICE_FOC_OUTPUT_STOPPED != p_motor->st_status.en_output_state)) ||
        (0U == p_motor->st_status.u8_hardware_ready) ||
        (CORE_GPIO_PIN_RESET != core_gpio_read_pin(CORE_GPIO_MOTOR1_EN)))
    {
        printf("[CAL] stop state invalid\r\n");
        return APP_STATUS_INVALID_PARAM;
    }
    if (!(FOC_M1_STOP_SPEED_RAD_S > 0.0f) ||
        !isfinite(FOC_M1_STOP_SPEED_RAD_S) ||
        (0U == FOC_M1_STOP_STABLE_MS) || (0U == FOC_M1_STOP_DECAY_MS) ||
        (2048U <= FOC_M1_STOP_POSITION_COUNTS) ||
        (1000U > FOC_M1_STOP_MAX_SAMPLE_GAP_US) ||
        (FOC_M1_STOP_STABLE_MS >= FOC_M1_STOP_TIMEOUT_MS) ||
        (FOC_M1_STOP_DECAY_MS >=
             (FOC_M1_STOP_TIMEOUT_MS - FOC_M1_STOP_STABLE_MS)))
    {
        printf("[CAL] stop limits invalid\r\n");

        return APP_STATUS_INVALID_PARAM;
    }
    //--------------------------参数检查 end--------------------------//
    if (CORE_ADC_OK != core_adc_register_injected_callback(CORE_ADC1, NULL))
    {
        return APP_STATUS_IO_ERROR;
    }
    p_motor->st_status.en_output_state = SERVICE_FOC_OUTPUT_STOPPING;
    u32_start = HAL_GetTick();
    while (HAL_I2C_STATE_READY != HAL_I2C_GetState(&hi2c1))
    {
        if (FOC_M1_STOP_DMA_TIMEOUT_MS <=
                (uint32_t)(HAL_GetTick() - u32_start))
        {
            return APP_STATUS_IO_ERROR;
        }
    }
    if (BSP_WRAPPER_ENCODER_OK !=
            bsp_wrapper_encoder_read_angle(&u16_previous))
    {
        return APP_STATUS_IO_ERROR;
    }
    u32_previous_us = core_time_get_us();
    u32_start = HAL_GetTick();
    u32_stable_start = u32_start;
    while (FOC_M1_STOP_TIMEOUT_MS > (uint32_t)(HAL_GetTick() - u32_start))
    {
        u32_now = core_time_get_us();
        u32_elapsed = core_time_get_us_delta(u32_now, u32_previous_us);
        if (1000U > u32_elapsed) { continue; }
        if ((BSP_WRAPPER_ENCODER_OK !=
                 bsp_wrapper_encoder_read_angle(&u16_raw)) ||
            (SERVICE_FOC_OUTPUT_STOPPING !=
                 p_motor->st_status.en_output_state))
        {
            return APP_STATUS_IO_ERROR;
        }
        u32_now = core_time_get_us();
        u32_elapsed = core_time_get_us_delta(u32_now, u32_previous_us);
        if (u32_elapsed > FOC_M1_STOP_MAX_SAMPLE_GAP_US)
        {
            return APP_STATUS_IO_ERROR;
        }
        en_current_status = bsp_wrapper_current_read(
            CORE_ADC2,                        /* 电机 1 电流 ADC */
            &p_motor->st_phase.f_current_a,    /* A 相电流/A */
            &p_motor->st_phase.f_current_b,    /* B 相电流/A */
            &p_motor->st_phase.f_current_c);   /* C 相电流/A */
        if (BSP_WRAPPER_CURRENT_OK == en_current_status)
        {
            if (!(fabsf(p_motor->st_phase.f_current_a) <=
                      p_motor->st_config.f_phase_overcurrent_a) ||
                !(fabsf(p_motor->st_phase.f_current_b) <=
                      p_motor->st_config.f_phase_overcurrent_a) ||
                !(fabsf(p_motor->st_phase.f_current_c) <=
                      p_motor->st_config.f_phase_overcurrent_a))
            {
                return APP_STATUS_IO_ERROR;
            }
        }
        else if (BSP_WRAPPER_CURRENT_ERR_STATE != en_current_status)
        {
            return APP_STATUS_IO_ERROR;
        }
        i32_delta = (int32_t)u16_raw - (int32_t)u16_previous;
        if (2048 < i32_delta) { i32_delta -= 4096; }
        if (-2048 > i32_delta) { i32_delta += 4096; }
        u16_previous = u16_raw;
        u32_previous_us = u32_now;
        i32_position += i32_delta;
        if (i32_min > i32_position) { i32_min = i32_position; }
        if (i32_max < i32_position) { i32_max = i32_position; }
        f_speed = fabsf((float)i32_delta) * (6.28318530718f / 4096.0f) *
            1000000.0f / (float)u32_elapsed;
        if ((f_speed > FOC_M1_STOP_SPEED_RAD_S) ||
            ((uint32_t)(i32_max - i32_min) > FOC_M1_STOP_POSITION_COUNTS))
        {
            u32_stable_start = HAL_GetTick();
            i32_position = 0;
            i32_min = 0;
            i32_max = 0;
        }
        else if (u32_required_ms <=
                     (uint32_t)(HAL_GetTick() - u32_stable_start))
        {
            u32_mask = __get_PRIMASK();
            __disable_irq();
            if (SERVICE_FOC_OUTPUT_STOPPING ==
                    p_motor->st_status.en_output_state)
            {
                p_motor->st_status.en_output_state = SERVICE_FOC_OUTPUT_STOPPED;
                __set_PRIMASK(u32_mask);
                return APP_STATUS_OK;
            }
            __set_PRIMASK(u32_mask);
            return APP_STATUS_IO_ERROR;
        }
    }
    return APP_STATUS_IO_ERROR;
}

/**
 * @brief 保存电角度零偏和编码器方向到 Flash 并读回验证。
 *
 * Steps:
 *  1. 校验编号、角度及方向，构造带方向标识和反码的 16 字节记录。
 *  2. 写入本电机记录并读回比较，失败时返回 IO 错误。
 *
 * @param[in] u8_motor_id : 电机编号，1 或 2。
 * @param[in] f_zero_deg : 电角度零偏，范围 [0, 360)，单位度。
 * @param[in] i8_direction : 自动标定的编码器方向，+1 或 -1。
 * @return app_status_t : 成功、参数错误或 Flash IO 错误。
 * @note 仅在主循环串行调用，所有电机输出须关闭；不保证掉电原子性。
 */
app_status_t service_foc_zero_save(uint8_t u8_motor_id, float f_zero_deg,
    int8_t i8_direction)
{
    uint32_t au32_record[4];
    uint32_t au32_check[4];
    uint32_t au32_current[4];
    uint32_t au32_current_check[4];
    uint32_t u32_addr;
    uint16_t u16_offset_a;
    uint16_t u16_offset_b;
    uint16_t u16_offset_c;

    //-------------------------参数检查 begin-------------------------//
    if (((1U != u8_motor_id) && (2U != u8_motor_id)) ||
        ((1 != i8_direction) && (-1 != i8_direction)) ||
        !(f_zero_deg >= 0.0f && f_zero_deg < 360.0f) ||
        (sizeof(float) != sizeof(uint32_t)))
    {
        return APP_STATUS_INVALID_PARAM;
    }
    //--------------------------参数检查 end--------------------------//

    /* 参数区前 32 字节：每台 16 字节，标识包含版本和方向。 */
    u32_addr = BSP_FLASH_BASE + ((uint32_t)u8_motor_id - 1U) * 16U;
    au32_record[0] = (1 == i8_direction)
        ? SERVICE_FOC_ZERO_TAG_FORWARD : SERVICE_FOC_ZERO_TAG_REVERSE;
    (void)memcpy(&au32_record[1], &f_zero_deg, sizeof(f_zero_deg));
    au32_record[2] = ~au32_record[1];
    au32_record[3] = ~au32_record[0];
    if ((BSP_FLASH_OK != bsp_wrapper_flash_write(u32_addr,
            sizeof(au32_record), (uint8_t *)au32_record)) ||
        (BSP_FLASH_OK != bsp_wrapper_flash_read(u32_addr,
            sizeof(au32_check), (uint8_t *)au32_check)) ||
        (0 != memcmp(au32_record, au32_check, sizeof(au32_record))))
    {
        return APP_STATUS_IO_ERROR;
    }
    bsp_wrapper_current_get_offset(
        &u16_offset_a, &u16_offset_b, &u16_offset_c);
    au32_current[0] = SERVICE_FOC_CURRENT_TAG;
    au32_current[1] = (uint32_t)u16_offset_a |
        ((uint32_t)u16_offset_b << 16U);
    au32_current[2] = (uint32_t)u16_offset_c;
    au32_current[3] = ~(au32_current[0] ^ au32_current[1] ^
        au32_current[2]);
    u32_addr = BSP_FLASH_BASE +
        ((uint32_t)u8_motor_id - 1U) * 16U +
        SERVICE_FOC_CURRENT_ADDR_OFF;
    if ((BSP_FLASH_OK != bsp_wrapper_flash_write(u32_addr,
            sizeof(au32_current), (uint8_t *)au32_current)) ||
        (BSP_FLASH_OK != bsp_wrapper_flash_read(u32_addr,
            sizeof(au32_current_check), (uint8_t *)au32_current_check)) ||
        (0 != memcmp(au32_current, au32_current_check,
            sizeof(au32_current))) ||
        (4095U < u16_offset_a) || (4095U < u16_offset_b) ||
        (4095U < u16_offset_c))
    {
        return APP_STATUS_IO_ERROR;
    }
    return APP_STATUS_OK;
}

/**
 * @brief 加载零偏和方向，旧版及无效记录保留默认值并标记未标定。
 *
 * Steps:
 *  1. 读取对应电机记录，检查版本标识、反码和角度范围。
 *  2. 仅有效记录覆盖输出；空白或损坏记录保留调用者的默认值。
 *
 * @param[in] u8_motor_id : 电机编号，1 或 2。
 * @param[in,out] p_zero_deg : 配置默认值，成功加载后为 Flash 值。
 * @param[in,out] p_direction : 默认方向，成功加载后为标定方向。
 * @param[out] p_valid : 1 为有效记录，空白/损坏/读取失败为 0。
 * @return app_status_t : 成功、参数错误或读取失败。
 */
app_status_t service_foc_zero_load(uint8_t u8_motor_id,
    float *p_zero_deg, int8_t *p_direction, uint8_t *p_valid)
{
    uint32_t au32_record[4];
    uint32_t au32_current[4];
    uint32_t u32_addr;
    float f_zero_deg;

    //-------------------------参数检查 begin-------------------------//
    if (NULL != p_valid) { *p_valid = 0U; }
    if ((NULL == p_zero_deg) || (NULL == p_direction) || (NULL == p_valid) ||
        ((1U != u8_motor_id) && (2U != u8_motor_id)) ||
        (sizeof(float) != sizeof(uint32_t)))
    {
        return APP_STATUS_INVALID_PARAM;
    }
    //--------------------------参数检查 end--------------------------//

    *p_valid = 0U;
    u32_addr = BSP_FLASH_BASE + ((uint32_t)u8_motor_id - 1U) * 16U;
    if (BSP_FLASH_OK != bsp_wrapper_flash_read(u32_addr,
            sizeof(au32_record), (uint8_t *)au32_record))
    {
        return APP_STATUS_IO_ERROR;
    }
    if (((SERVICE_FOC_ZERO_TAG_FORWARD == au32_record[0]) ||
         (SERVICE_FOC_ZERO_TAG_REVERSE == au32_record[0])) &&
        (~au32_record[0] == au32_record[3]) &&
        (~au32_record[1] == au32_record[2]))
    {
        (void)memcpy(&f_zero_deg, &au32_record[1], sizeof(f_zero_deg));
        if (f_zero_deg >= 0.0f && f_zero_deg < 360.0f)
        {
            *p_zero_deg = f_zero_deg;
            *p_direction = (SERVICE_FOC_ZERO_TAG_FORWARD == au32_record[0])
                ? 1 : -1;
            *p_valid = 1U;

            u32_addr += SERVICE_FOC_CURRENT_ADDR_OFF;
            if ((BSP_FLASH_OK != bsp_wrapper_flash_read(u32_addr,
                    sizeof(au32_current), (uint8_t *)au32_current)) ||
                (SERVICE_FOC_CURRENT_TAG != au32_current[0]) ||
                (au32_current[3] != ~(au32_current[0] ^
                    au32_current[1] ^ au32_current[2])) ||
                (4095U < (au32_current[1] & 0xFFFFU)) ||
                (4095U < (au32_current[1] >> 16U)) ||
                (4095U < (au32_current[2] & 0xFFFFU)))
            {
                *p_valid = 0U;
                return APP_STATUS_OK;
            }
            bsp_wrapper_current_set_offset(
                (uint16_t)(au32_current[1] & 0xFFFFU),
                (uint16_t)(au32_current[1] >> 16U),
                (uint16_t)(au32_current[2] & 0xFFFFU));
        }
    }
    return APP_STATUS_OK;
}

/**
 * @brief 查询电机 1 是否正在进行零点标定。
 *
 * Steps:
 *  1. 检查输出指针并读取标定忙标志。
 *
 * @param[out] p_calibrating : 0 表示空闲，1 表示正在标定。
 * @return app_status_t : 成功或参数错误。
 */
app_status_t service_foc_zero_get_calibrating(uint8_t *p_calibrating)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_calibrating)
    {
        return APP_STATUS_INVALID_PARAM;
    }
    //--------------------------参数检查 end--------------------------//

    *p_calibrating = g_u8_motor1_calibrating;
    return APP_STATUS_OK;
}

/**
 * @brief 在主循环阻塞标定电机 1 电角度零偏，期间暂停正常 FOC。
 *
 * Steps:
 *  1. 阻塞停机并校准电流零偏，再施加零度电压矢量。
 *  2. 保持矢量，采集编码器并以圆周平均求机械零点。
 *  3. 改为正 90 度矢量，验证位移幅度并自动识别方向。
 *  4. 关闭 PWM，验证成功后将零偏和方向一起保存到 Flash 和 RAM。
 *  5. 流程结束前向事件队列投递 EVENT_CALIBRATION_EXIT。
 *
 * @return app_status_t : 成功、参数错误或采样/输出/对齐验证失败。
 * @note 只在硬件初始化后、全局中断开启的主循环调用，先关断并等停稳。
 *       幅值和时间在函数开头修改；成功后写入 Flash，不自动恢复输出。
 *       PWM 比较值映射须实测确认，固定电压不代表闭环恒流。
 *       退出事件由本函数负责投递，调用方无需再处理退出流程。
 */
app_status_t service_foc_motor1_calibrate(void)
{
    st_service_foc_motor_t *p_motor = NULL;

    /* 配置对齐电压/V；0 度稳定 500ms、采样 500ms，90 度等待 1s。 */
    const float f_align_voltage_v = FOC_M1_CAL_ALIGN_VOLTAGE_V;
    svpwm_input_t st_in;
    svpwm_output_t st_out;
    app_status_t status = APP_STATUS_IO_ERROR;
    uint32_t u32_start;
    uint32_t u32_t0;
    uint32_t u32_count = 0U;
    uint16_t u16_raw;
    uint8_t u8_stage;
    float f_sum_sin = 0.0f;
    float f_sum_cos = 0.0f;
    float f_start_rad = 0.0f;
    float f_angle_rad = 0.0f;
    float f_step_rad = 0.0f;
    float f_expected_rad = 0.0f;
    float f_zero_deg = 0.0f;
    int8_t i8_direction = 0; /* 位移验证通过后才识别并保存方向。 */

    u32_t0 = HAL_GetTick();
    printf("[CAL] begin\r\n");

    //-------------------------参数检查 begin-------------------------//
    status = service_foc_motor1_get_state(&p_motor);
    if (APP_STATUS_OK != status)
    {
        return status;
    }
    status = APP_STATUS_IO_ERROR;
    /* 阻塞标定需要主循环及正常时基，接受请求后先关闭运行输出。 */
    if ((0U != __get_IPSR()) || (0U != __get_PRIMASK()) ||
        (0U != __get_BASEPRI()))
    {
        printf("[CAL] invalid call context\r\n");
        return APP_STATUS_INVALID_PARAM;
    }
    if (1 == g_u8_motor1_calibrating)
    {
        printf("[CAL] busy\r\n");
        return APP_STATUS_INVALID_PARAM;
    }
    if (0U == p_motor->st_config.u8_pole_pairs)
    {
        printf("[CAL] pole pairs invalid\r\n");
        return APP_STATUS_INVALID_PARAM;
    }
    if (!(p_motor->st_config.f_phase_overcurrent_a > 0.0f) ||
        !isfinite(p_motor->st_config.f_phase_overcurrent_a) ||
        !(f_align_voltage_v > 0.0f) ||
        !(f_align_voltage_v < p_motor->st_feedback.f_bus_voltage_v * 0.577350269f))
    {
        printf("[CAL] voltage/current limits invalid\r\n");
        return APP_STATUS_INVALID_PARAM;
    }
    //--------------------------参数检查 end--------------------------//
    //-------------------------前期检查 begin-------------------------//
    //取消回调注册,防止执行电流环计算
    (void)core_adc_register_injected_callback(CORE_ADC1, NULL);
    g_u8_motor1_calibrating = 1U;
    status = service_foc_motor1_pwm_enable(0U);//关驱动能力

    if (APP_STATUS_OK != status)
    {//关驱动失败
        printf("[CAL] disable failed status=%d\r\n", (int)status);
        goto cleanup;
    }
    status = service_foc_motor1_wait_stop();//5s内等待停稳+电流衰减
    if (APP_STATUS_OK != status)
    {//一定要停稳才开始检测
        printf("[CAL] wait stop failed status=%d\r\n", (int)status);
        goto cleanup;
    }
    //--------------------------前期准备 end--------------------------//
    status = APP_STATUS_IO_ERROR;
    {//计算并校验零偏稳定性，合格后写入电流采样模块供后续电流换算使用。
        typedef struct
        {
            uint32_t u32_sum;
            uint32_t u32_half_sum;
            uint16_t u16_min;
            uint16_t u16_max;
        } st_zero_sample_t;
        st_zero_sample_t ast_sample[3] =
        {
            {0U, 0U, 4095U, 0U},
            {0U, 0U, 4095U, 0U},
            {0U, 0U, 4095U, 0U}
        };
        uint16_t au16_offset[3];
        uint16_t u16_adc;
        uint32_t u32_sample;
        uint32_t u32_phase;
        uint32_t u32_previous = core_time_get_us();
        uint32_t u32_now;
        uint32_t u32_elapsed;
        uint32_t u32_half_delta;
        const uint32_t au32_rank[3] =
        {
            ADC_INJECTED_RANK_1,
            ADC_INJECTED_RANK_2,
            ADC_INJECTED_RANK_3
        };
        const uint16_t au16_limit_min[3] =
            {FOC_M1_ZERO_MIN_A, FOC_M1_ZERO_MIN_B, FOC_M1_ZERO_MIN_C};
        const uint16_t au16_limit_max[3] =
            {FOC_M1_ZERO_MAX_A, FOC_M1_ZERO_MAX_B, FOC_M1_ZERO_MAX_C};

        u32_start = HAL_GetTick();
        for (u32_sample = 0U; u32_sample < 1000U; u32_sample++)
        {
            do
            {
                u32_now = core_time_get_us();
                if (500U <= (uint32_t)(HAL_GetTick() - u32_start))
                {
                    printf("[CAL] zero sampling timeout\r\n");
                    goto cleanup;
                }
                u32_elapsed = core_time_get_us_delta(u32_now, u32_previous);
            } while (100U > u32_elapsed);
            if (u32_elapsed > FOC_M1_ZERO_MAX_SAMPLE_GAP_US)
            {
                printf("[CAL] zero sample gap=%lu us\r\n",
                    (unsigned long)u32_elapsed);
                goto cleanup;
            }
            u32_previous = u32_now;
            for (u32_phase = 0U; u32_phase < 3U; u32_phase++)
            {
                if (CORE_ADC_OK != core_adc_get_injected_value(
                        CORE_ADC2,                           /* 电流 ADC */
                        au32_rank[u32_phase],               /* 相序 */
                        &u16_adc))                           /* 原始值 */
                {
                    printf("[CAL] zero ADC read failed\r\n");
                    goto cleanup;
                }
                ast_sample[u32_phase].u32_sum += u16_adc;
                if (500U > u32_sample)
                {
                    ast_sample[u32_phase].u32_half_sum += u16_adc;
                }
                if (ast_sample[u32_phase].u16_min > u16_adc)
                {
                    ast_sample[u32_phase].u16_min = u16_adc;
                }
                if (ast_sample[u32_phase].u16_max < u16_adc)
                {
                    ast_sample[u32_phase].u16_max = u16_adc;
                }
            }
        }
        for (u32_phase = 0U; u32_phase < 3U; u32_phase++)
        {
            st_zero_sample_t *p_sample = &ast_sample[u32_phase];
            au16_offset[u32_phase] =
                (uint16_t)(p_sample->u32_sum / 1000U);
            u32_half_delta = (p_sample->u32_sum >=
                (2U * p_sample->u32_half_sum)) ?
                p_sample->u32_sum - 2U * p_sample->u32_half_sum :
                2U * p_sample->u32_half_sum - p_sample->u32_sum;
            if ((p_sample->u16_min < au16_limit_min[u32_phase]) ||
                (p_sample->u16_max > au16_limit_max[u32_phase]) ||
                ((uint32_t)(p_sample->u16_max - p_sample->u16_min) >
                    FOC_M1_ZERO_MAX_SPAN_COUNTS) ||
                (u32_half_delta > (500U * FOC_M1_ZERO_MAX_DRIFT_COUNTS)))
            {
                printf("[CAL] zero limits failed phase=%u\r\n",
                    (unsigned int)u32_phase);
                goto cleanup;
            }
        }
        bsp_wrapper_current_set_offset(
            au16_offset[0],  /* A 相零偏 */
            au16_offset[1],  /* B 相零偏 */
            au16_offset[2]); /* C 相零偏 */
    }

    /* 2. 共用 SVPWM 参数；时间比例归一化，不改变实际 PWM 频率。 */
    st_in.inv_v_dc = p_motor->st_config.f_inv_bus_voltage;
    st_in.t_pwm = 1.0f;
    st_in.k0 = 0.5f;
    st_in.arr = p_motor->st_config.u16_pwm_period;

    /* 两个阶段各 1 秒：阶段 0 为零度对齐，阶段 1 为正 90 度验证。 */
    for (u8_stage = 0U; u8_stage < 2U; u8_stage++)
    {
        float af_current_sum[3] = {0.0f, 0.0f, 0.0f};
        uint32_t u32_current_count = 0U;

        st_in.u_alpha = (0U == u8_stage) ? f_align_voltage_v : 0.0f;
        st_in.u_beta = (0U == u8_stage) ? 0.0f : f_align_voltage_v;
        if ((SVPWM_OK != svpwm_calc(&st_in, &st_out)) ||
            (CORE_TIME_OK != core_hrtim_set_motor_cmp(1U,
                st_out.ccr_u, st_out.ccr_v, st_out.ccr_w)))
        {
            printf("[CAL] stage%u PWM setup failed\r\n",
                (unsigned int)u8_stage);
            goto cleanup;
        }

        /* 预装载值生效后开启输出，第二阶段保持输出连续。 */
        HAL_Delay(1U);
        if ((0U == u8_stage) && (APP_STATUS_OK !=
            service_foc_motor1_pwm_enable(1U)))
        {
            printf("[CAL] PWM enable failed\r\n");
            goto cleanup;
        }

        u32_start = HAL_GetTick();
        do
        {
            HAL_Delay(5U);
            if (SERVICE_FOC_OUTPUT_ENABLED != p_motor->st_status.en_output_state) { goto cleanup; }
            /* 更新电机电流，超限退出；轮询检查不替代硬件过流保护。 */
            if (BSP_WRAPPER_CURRENT_OK != bsp_wrapper_current_read(CORE_ADC2,
                    &p_motor->st_phase.f_current_a,
                    &p_motor->st_phase.f_current_b,
                    &p_motor->st_phase.f_current_c))
            {
                printf("[CAL] current read failed\r\n");
                goto cleanup;
            }
            if (!(fabsf(p_motor->st_phase.f_current_a) <=
                    p_motor->st_config.f_phase_overcurrent_a) ||
                !(fabsf(p_motor->st_phase.f_current_b) <=
                    p_motor->st_config.f_phase_overcurrent_a) ||
                !(fabsf(p_motor->st_phase.f_current_c) <=
                    p_motor->st_config.f_phase_overcurrent_a))
            {
                printf("[CAL] overcurrent\r\n");
                goto cleanup;
            }
            /* 两阶段均取后 500ms 三相实测均值，不使用重构电流。 */
            if (500U <= (uint32_t)(HAL_GetTick() - u32_start))
            {
                af_current_sum[0] += p_motor->st_phase.f_current_a;
                af_current_sum[1] += p_motor->st_phase.f_current_b;
                af_current_sum[2] += p_motor->st_phase.f_current_c;
                u32_current_count++;
            }
            /* 零度阶段前 500ms 只等待稳定，随后开始圆周平均。 */
            if ((0U == u8_stage) &&
                (500U > (uint32_t)(HAL_GetTick() - u32_start)))
            {
                continue;
            }
            if (BSP_WRAPPER_ENCODER_OK !=
                bsp_wrapper_encoder_read_angle(&u16_raw))
            {
                printf("[CAL] encoder read failed\r\n");
                goto cleanup;
            }
            /* 使用原始机械角度，方向在正 90 度位移验证后自动识别。 */
            f_angle_rad = (float)u16_raw * (6.28318530718f / 4096.0f);
            /* 仅零度对齐阶段累加正余弦及样本数，之后用 atan2 求均值；
             * 避免 359 度和 1 度直接平均得到 180 度的跨零错误。 */
            if (0U == u8_stage)
            {
                f_sum_sin += sinf(f_angle_rad);
                f_sum_cos += cosf(f_angle_rad);
                u32_count++;
            }
        } while (1000U > (uint32_t)(HAL_GetTick() - u32_start));

        /* 主循环每阶段只输出一次；诊断换算为 (ADC-零偏)*正系数。 */
        if (0U != u32_current_count)
        {
            printf("[CAL-I] angle=%u n=%lu Ia=%.4f Ib=%.4f Ic=%.4f A\r\n",
                (unsigned int)(u8_stage * 90U),
                (unsigned long)u32_current_count,
                (double)(af_current_sum[0] / (float)u32_current_count),
                (double)(af_current_sum[1] / (float)u32_current_count),
                (double)(af_current_sum[2] / (float)u32_current_count));
        }

        /* 零度阶段结束：样本须集中，圆周平均避免跨 0 度的均值错误。 */
        if (0U == u8_stage)
        {
            if ((2U > u32_count) ||
                ((f_sum_sin * f_sum_sin + f_sum_cos * f_sum_cos) <
                 (0.99f * (float)u32_count * (float)u32_count)))
            {
                printf("[CAL] alignment unstable\r\n");
                goto cleanup;
            }
            f_start_rad = atan2f(f_sum_sin, f_sum_cos);
        }
    }

    /* 正 90 度矢量的位移绝对值须接近 90/极对数，符号决定方向。 */
    f_step_rad = f_angle_rad - f_start_rad;
    while (f_step_rad > 3.14159265359f) { f_step_rad -= 6.28318530718f; }
    while (f_step_rad < -3.14159265359f) { f_step_rad += 6.28318530718f; }
    f_expected_rad = 1.57079632679f / (float)p_motor->st_config.u8_pole_pairs;

    if (!(fabsf(f_step_rad) > 0.5f * f_expected_rad &&
          fabsf(f_step_rad) < 1.5f * f_expected_rad))
    {
        printf("[CAL] move invalid step=%.4f expected=%.4f rad\r\n",
            (double)f_step_rad, (double)f_expected_rad);
        goto cleanup;
    }

    i8_direction = (f_step_rad > 0.0f) ? 1 : -1;

    /* 电角度 = 原始机械角度 * 方向 * 极对数 + 零偏。 */
    f_zero_deg = -f_start_rad / SERVICE_FOC_DEG_TO_RAD *
        (float)i8_direction * (float)p_motor->st_config.u8_pole_pairs;
    while (f_zero_deg < 0.0f) { f_zero_deg += 360.0f; }
    while (f_zero_deg >= 360.0f) { f_zero_deg -= 360.0f; }
    printf("[CAL] direction=%d zero=%.3f deg\r\n",
        (int)i8_direction, (double)f_zero_deg);
    status = APP_STATUS_OK;

cleanup:
    /* 共用关闭入口：立即关断并阻塞确认停机；失败不发布新角度。 */
    if (APP_STATUS_OK != service_foc_motor1_pwm_enable(0U))
    {
        status = APP_STATUS_IO_ERROR;
    }
    /* 硬件输出关闭后持久化，写入并验证成功才发布新零偏。 */
    if (APP_STATUS_OK == status)
    {
        status = service_foc_zero_save(1U, f_zero_deg, i8_direction);
        printf("[CAL] flash save status=%d\r\n", (int)status);
    }
    if (APP_STATUS_OK == status)
    {
        p_motor->st_config.f_electrical_zero_deg = f_zero_deg;
        p_motor->st_config.i8_encoder_direction = i8_direction;
        p_motor->st_status.u8_electrical_zero_valid = 1U;
        p_motor->st_status.u8_angle_update_pending = 0U;
        (void)current_loop_reset(
            &p_motor->st_control.st_current_loop);
    }
    g_u8_motor1_calibrating = 0U;
    if (0U != p_motor->st_status.u8_hardware_ready)
    {
        bsp_wrapper_encoder_reset_feedback_history();
        (void)core_adc_register_injected_callback(
            CORE_ADC1, service_foc_motor1_foc_task);
    }

    /* 标定流程结束，投递退出事件让应用状态机回到普通模式。
     * 成功失败都投递：失败时状态机仍在普通模式，本事件查表无匹配
     * 会被丢弃，不会掩盖真实结果；返回值只反映标定结果。 */
    /* 标定成功后开启速度控制，失败时保持输出关闭。 */
    if ((APP_STATUS_OK == status) &&
        (0U != p_motor->st_status.u8_hardware_ready))
    {
        status = service_foc_motor1_pwm_enable(1U);
        printf("[CAL] resume enable status=%d\r\n", (int)status);
    }
    (void)queue_push(&g_msg_queue, EVENT_CALIBRATION_EXIT);
    printf("[CAL] end status=%d elapsed=%lu ms\r\n",
        (int)status, (unsigned long)(HAL_GetTick() - u32_t0));
    return status;
}
//************************* Function Definitions end *************************//
