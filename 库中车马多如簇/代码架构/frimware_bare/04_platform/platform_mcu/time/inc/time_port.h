/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file time_port.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief 提供 MCU 毫秒时间获取接口。
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
#include <stdint.h>
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//
uint32_t core_time_get_direct(void);
uint32_t core_time_get(void);
void board_delay_us(uint32_t delay_us);
//************************* Function Prototypes end **************************//

#endif /* TIME_PORT_H */

//******************************* End of File ********************************//
