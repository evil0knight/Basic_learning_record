/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file main.c
 *
 * @par dependencies
 * - app_event_flag.h
 * - app_state.h
 * - msg_dispatcher.h
 * - msg_queue.h
 *
 * @author YA
 *
 * @brief 表驱法裸机框架：main.c
 *
 * 处理流程：中断入队，主循环按消息表与状态表处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes*********************************//
#include "app_event_flag.h"
#include "app_state.h"
#include "msg_dispatcher.h"
#include "msg_queue.h"
//********************************Includes*********************************//

/**
 * @brief 初始化并运行裸机消息循环。
 *
 * @note 无输入参数。
 * @return int：非零表示初始化或运行失败。
 */
int main(void)
{
    uint32_t u32_flags = 0U;

    /* 平台先初始化时钟，框架初始化完成后再使能接收中断。 */
    if ((APP_STATUS_OK != queue_init(&g_msg_queue)) ||
        (APP_STATUS_OK != app_state_init())) {
        return 1;
    }
    while (1) {
        if (APP_STATUS_OK !=
            get_event_flag(APP_EVENT_MESSAGE_READY, &u32_flags)) {
            return 1;
        }
        if (0U != u32_flags) {
            (void)clear_event_flag(APP_EVENT_MESSAGE_READY);
            (void)do_read_message();
        }
    }
}
