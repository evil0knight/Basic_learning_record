/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file time_port.c
 *
 * @par dependencies
 * - time_port.h
 * - main.h
 * - hrtim.h
 *
 * @author YA
 *
 * @brief 基于 HAL 和 DWT 实现时间获取及定时器启动接口。
 *
 * Processing flow:
 *
 * core_time_get -> core_time_get_direct -> HAL_GetTick。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "time_port.h"

#include "main.h"
#include "hrtim.h"
//******************************* Includes end *******************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 初始化 DWT 周期计数器。
 *
 * Steps:
 *  1. 使能内核跟踪功能。
 *  2. 清零并使能 DWT 周期计数器，返回成功状态。
 *
 * @return core_time_status_t : 初始化完成返回 CORE_TIME_OK。
 * @note 应在系统时钟配置完成后调用，重复调用会清零计数器。
 */
core_time_status_t core_time_us_init(void)
{
    //-------------------------参数检查 begin-------------------------//
    /* 无输入参数；由调用者保证系统时钟已配置。 */
    //--------------------------参数检查 end--------------------------//

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    return CORE_TIME_OK;
}

/**
 * @brief 启动 HRTIM1 主定时器计数。
 *
 * Steps:
 *  1. 检查 HRTIM1 句柄是否已初始化。
 *  2. 启动主定时器计数并返回操作状态。
 *
 * @return core_time_status_t : 成功、未初始化、忙、超时或错误状态。
 * @note 使用前调用 MX_HRTIM1_Init；此接口不使能中断或 PWM 输出。
 */
core_time_status_t core_hrtim_master_start(void)
{
    HAL_StatusTypeDef en_hal_status;

    //-------------------------参数检查 begin-------------------------//
    if ((HRTIM1 != hhrtim1.Instance) ||
        (HAL_HRTIM_STATE_RESET == hhrtim1.State))
    {
        return CORE_TIME_NOT_INIT;
    }
    //--------------------------参数检查 end--------------------------//

    en_hal_status = HAL_HRTIM_WaveformCounterStart(
        &hhrtim1, HRTIM_TIMERID_MASTER);

    switch (en_hal_status)
    {
        case HAL_OK:
            return CORE_TIME_OK;

        case HAL_BUSY:
            return CORE_TIME_BUSY;

        case HAL_TIMEOUT:
            return CORE_TIME_TIMEOUT;

        default:
            return CORE_TIME_ERROR;
    }
}

/**
 * @brief 启动普通定时器计数并使能更新中断。
 *
 * Steps:
 *  1. 检查定时器句柄、实例和初始化状态。
 *  2. 启动定时器计数及更新中断，返回操作状态。
 *
 * @param[in,out] p_tim : 已完成 HAL 基础定时器初始化的句柄。
 *
 * @return core_time_status_t : 成功、参数无效、未初始化或 HAL 操作状态。
 * @note 调用前须配置周期、NVIC 及 IRQ 中的 HAL_TIM_IRQHandler。
 * @note 从模式使用触发启动时，计数仍由硬件触发信号启动。
 */
core_time_status_t core_tim_start(TIM_HandleTypeDef * const p_tim)
{
    HAL_StatusTypeDef en_hal_status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_tim)
    {
        return CORE_TIME_INVALID_PARAM;
    }

    if (HAL_TIM_STATE_RESET == p_tim->State)
    {
        return CORE_TIME_NOT_INIT;
    }

    if (!IS_TIM_INSTANCE(p_tim->Instance))
    {
        return CORE_TIME_INVALID_PARAM;
    }

    if (HAL_TIM_STATE_BUSY == p_tim->State)
    {
        return CORE_TIME_BUSY;
    }
    //--------------------------参数检查 end--------------------------//

    en_hal_status = HAL_TIM_Base_Start_IT(p_tim);

    switch (en_hal_status)
    {
        case HAL_OK:
            return CORE_TIME_OK;

        case HAL_BUSY:
            return CORE_TIME_BUSY;

        case HAL_TIMEOUT:
            return CORE_TIME_TIMEOUT;

        default:
            return CORE_TIME_ERROR;
    }
}

/**
 * @brief 获取 DWT 周期计数器换算的微秒时间。
 *
 * 步骤：
 *  1. 读取 DWT 周期计数器。
 *  2. 按内核时钟频率换算并返回微秒计数。
 *
 * @return uint32_t : 当前微秒计数，随 32 位 DWT 周期计数器回绕。
 * @note 使用前调用 core_time_us_init，SystemCoreClock 须正确且不低于 1 MHz。
 */
uint32_t core_time_get_us(void)
{
    return DWT->CYCCNT / (SystemCoreClock / 1000000U);
}

/**
 * @brief 计算两个 DWT 微秒时间戳的差值，处理计数器回绕。
 * @param[in] u32_current_us : 当前微秒时间戳。
 * @param[in] u32_previous_us : 上一次微秒时间戳。
 * @return uint32_t : 时间差，单位 us。
 */
uint32_t core_time_get_us_delta(
    uint32_t u32_current_us,
    uint32_t u32_previous_us)
{
    uint32_t u32_cycles_per_us = SystemCoreClock / 1000000U;
    uint32_t u32_wrap_us;

    if (0U == u32_cycles_per_us)
    {
        return 0U;
    }
    u32_wrap_us = (UINT32_MAX / u32_cycles_per_us) + 1U;
    if (u32_current_us >= u32_previous_us)
    {
        return u32_current_us - u32_previous_us;
    }
    return (u32_wrap_us - u32_previous_us) + u32_current_us;
}

/**
 * @brief 直接获取 HAL 时间基准的当前毫秒计数。
 *
 * Steps:
 *  1. 调用 HAL_GetTick 读取时间基准。
 *  2. 直接返回毫秒计数。
 *
 * @return uint32_t : 当前毫秒计数，超过 UINT32_MAX 后回绕。
 */
uint32_t core_time_get_direct(void)
{
    return HAL_GetTick();
}

/**
 * @brief 通过统一接口获取当前毫秒计数。
 *
 * Steps:
 *  1. 调用 core_time_get_direct 获取当前时间。
 *  2. 返回毫秒计数。
 *
 * @return uint32_t : 当前毫秒计数，超过 UINT32_MAX 后回绕。
 */
uint32_t core_time_get(void)
{
    return core_time_get_direct();
}

/**
 * @brief 使用 DWT 周期计数器实现微秒级忙等延时。
 *
 * 步骤：
 *  1. 确保 DWT 周期计数器已使能。
 *  2. 按 CPU 主频换算出目标周期数并等待。
 *
 * @param[in]  delay_us             : 延时时间，单位微秒。
 *
 * @return void : 无返回值。
 */
void core_delay_us(uint32_t delay_us)
{
    uint32_t cycles;
    uint32_t start;

    //-------------------------参数检查 begin-------------------------//
    if (delay_us == 0U)
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    cycles = delay_us * (SystemCoreClock / 1000000U);
    start = DWT->CYCCNT;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles)
    {
    }
}

//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
