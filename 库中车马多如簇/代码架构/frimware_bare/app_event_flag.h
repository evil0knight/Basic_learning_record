/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file app_event_flag.h
 *
 * @par dependencies
 * - app_event.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 表驱法裸机框架：app_event_flag.h
 *
 * 处理流程：中断入队，主循环按消息表与状态表处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef APP_EVENT_FLAG_H
#define APP_EVENT_FLAG_H

//********************************Includes*********************************//
#include "app_event.h"

#include <stdint.h>
//********************************Includes*********************************//

#define APP_EVENT_MESSAGE_READY 0x01U

/**
 * @brief 设置事件位。
 *
 * @param[in] u32_flag 事件位掩码。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t set_event_flag(uint32_t u32_flag);
/**
 * @brief 清除事件位。
 *
 * @param[in] u32_flag 事件位掩码。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t clear_event_flag(uint32_t u32_flag);
/**
 * @brief 通过输出参数读取事件位。
 *
 * @param[in] u32_flag 事件位掩码。
 * @param[out] p_u32_value 事件位输出指针。
 * @return app_status_t：成功或具体错误状态。
 */
app_status_t get_event_flag(
    uint32_t u32_flag, uint32_t * const p_u32_value);

#endif
