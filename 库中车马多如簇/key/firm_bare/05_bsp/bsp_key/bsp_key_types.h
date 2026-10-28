/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_key_types.h
 * @par dependencies
 * - stdint.h
 * @author YA
 * @brief 定义按键状态码、动作及中断回调类型。
 * Processing flow:
 * 识别按键动作 -> 保存事件位 -> 主循环读取。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef BSP_KEY_TYPES_H
#define BSP_KEY_TYPES_H

#include <stdint.h>

typedef enum
{
    BSP_KEY_OK = 0,
    BSP_KEY_ERR_ARGUMENT,
    BSP_KEY_ERR_STATE,
    BSP_KEY_ERR_IO,
    BSP_KEY_ERR_BUSY
} en_key_status_t;

typedef enum
{
    BSP_KEY_EVENT_PRESS = 0,
    BSP_KEY_EVENT_RELEASE,
    BSP_KEY_EVENT_SHORT_PRESS,
    BSP_KEY_EVENT_LONG_PRESS,
    BSP_KEY_EVENT_MAX
} en_key_event_t;

typedef en_key_status_t (*pf_key_irq_callback_t)(void *p_object);

#endif /* BSP_KEY_TYPES_H */
