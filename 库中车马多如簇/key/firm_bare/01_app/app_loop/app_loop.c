/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file app_loop.c
 *
 * @par dependencies
 * - app_loop.h
 * - ../app_isr_handlers/user_isr_handlers.h
 * - bsp_wrapper_key.h
 * - msg_queue.h
 *
 * @author YA
 *
 * @brief 主循环后台轮询与定时器 ISR 回调实现。
 *
 * Processing flow:
 *
 * app_loop_init() 挂载 do_loop_tick 到 pf_tim_interrupt_callback；
 * do_loop() 由主循环调用执行后台轮询；
 * do_loop_tick() 由定时器 ISR 通过回调指针调用。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "app_loop.h"

#include "../app_isr_handlers/user_isr_handlers.h"
#include "bsp_wrapper_key.h"
#include "msg_queue.h"

/* 表索引为按键 ID；增加按键时同步添加 App 事件和映射。 */
static const app_event_t g_a_key_events[][BSP_KEY_EVENT_MAX] =
{
    {EVENT_KEY0_PRESS, EVENT_KEY0_RELEASE,
     EVENT_KEY0_SHORT_PRESS, EVENT_KEY0_LONG_PRESS}
};

/**
 * @brief 初始化主循环层，挂载定时器 ISR 回调指针。
 *
 * Steps:
 *  1. 将 do_loop_tick 写入 pf_tim_interrupt_callback。
 *
 * @return app_status_t : 操作状态。
 */
app_status_t app_loop_init(void)
{
    //-------------------------参数检查 begin-------------------------//
    /* 无入参，挂载框架的定时器回调。 */
    //--------------------------参数检查 end--------------------------//
    pf_tim_interrupt_callback = do_loop_tick;
    return APP_STATUS_OK;
}

/**
 * @brief 主循环后台轮询入口，与 do_read_message 并列调用。
 *
 * Steps:
 *  1. 通过 Wrapper 扫描并读取按键事件。
 *  2. 映射为 App 事件并投递队列。
 *
 * @return app_status_t : 操作状态。
 */
app_status_t do_loop(void)
{
    uint32_t u32_key_id;
    uint32_t u32_event;
    uint32_t u32_flags;
    const uint32_t u32_key_count =
        sizeof(g_a_key_events) / sizeof(g_a_key_events[0]);
    app_status_t en_status;
    app_status_t en_result;

    //-------------------------参数检查 begin-------------------------//
    /* 无入参；Wrapper 检查初始化状态和按键 ID。 */
    //--------------------------参数检查 end--------------------------//

    en_result = (BSP_KEY_OK == bsp_wrapper_key_scan())
        ? APP_STATUS_OK : APP_STATUS_IO_ERROR;
    for (u32_key_id = 0U; u32_key_id < u32_key_count; u32_key_id++)
    {
        if (BSP_KEY_OK != bsp_wrapper_key_take_events(u32_key_id, &u32_flags))
        {
            if (APP_STATUS_OK == en_result)
            {
                en_result = APP_STATUS_IO_ERROR;
            }
            continue;
        }
        for (u32_event = 0U; u32_event < BSP_KEY_EVENT_MAX; u32_event++)
        {
            if (0U == (u32_flags & (UINT32_C(1) << u32_event)))
            {
                continue;
            }
            en_status = queue_push(&g_msg_queue,
                g_a_key_events[u32_key_id][u32_event]);
            if (APP_STATUS_OK == en_result)
            {
                en_result = en_status;
            }
        }
    }
    return en_result;
}

/**
 * @brief 定时器 ISR 回调，用于高频抢占执行。
 *
 * Steps:
 *  1. 执行周期性关键处理（具体工程接入时填充）。
 *
 * @param[in] p_object : ISR 上下文对象，当前未使用。
 * @param[in] p_data : ISR 上下文数据，当前未使用。
 */
void do_loop_tick(void *p_object, void *p_data)
{
    //-------------------------参数检查 begin-------------------------//
    /* 框架保留入口，上下文未使用，允许为空。 */
    (void)p_object;
    (void)p_data;
    //--------------------------参数检查 end--------------------------//
    //----------------------------业务填充begin----------------------------//
    // 具体工程接入时在此添加 ISR 内高频处理（如 PID，内部用 DWT 读 Δt）。
    //-----------------------------业务填充end-----------------------------//
}
