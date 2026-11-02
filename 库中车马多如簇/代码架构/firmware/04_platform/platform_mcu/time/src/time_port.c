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
 *
 * @author YA
 *
 * @brief 基于 HAL 实现 MCU 毫秒时间获取接口。
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
//******************************* Includes end *******************************//

//************************ Function Definitions begin ************************//
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
void board_delay_us(uint32_t delay_us)
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
