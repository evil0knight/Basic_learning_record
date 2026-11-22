/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file time_port.h
 *
 * @par dependencies
 * - stm32g4xx_hal.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 提供 MCU 时间获取及定时器启动接口。
 *
 * Processing flow:
 *
 * 调用时间接口 -> 获取 HAL 毫秒计数 -> 返回当前时间。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef TIME_PORT_H
#define TIME_PORT_H

//****************************** Includes begin ******************************//
#include "stm32g4xx_hal.h"

#include <stdint.h>
//******************************* Includes end *******************************//

//****************************** Enums begin ********************************//
/**
 * @brief 时间模块操作状态。
 */
typedef enum
{
    CORE_TIME_OK = 0,
    CORE_TIME_ERROR,
    CORE_TIME_BUSY,
    CORE_TIME_TIMEOUT,
    CORE_TIME_INVALID_PARAM,
    CORE_TIME_NOT_INIT
} core_time_status_t;
//******************************* Enums end *********************************//

//************************ Function Prototypes begin *************************//
core_time_status_t core_time_us_init(void);
core_time_status_t core_hrtim_master_start(void);
core_time_status_t core_tim_start(TIM_HandleTypeDef * const p_tim);
uint32_t core_time_get_us(void);
uint32_t core_time_get_us_delta(uint32_t u32_current_us,
                                uint32_t u32_previous_us);
uint32_t core_time_get_direct(void);
uint32_t core_time_get(void);
void core_delay_us(uint32_t delay_us);
//************************* Function Prototypes end **************************//

#endif /* TIME_PORT_H */

//******************************* End of File ********************************//
