/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file app_loop.c
 *
 * @par dependencies
 * - app_loop.h
 * - 无额外依赖
 *
 * @author YA
 *
 * @brief 主循环后台轮询与定时器 ISR 回调实现。
 *
 * Processing flow:
 *
 * app_loop_init() 初始化主循环层；
 * do_loop() 由主循环调用执行后台轮询；
 * do_loop_tick() 由 App 的定时器 HAL 回调直接调用。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "app_loop.h"



/**
 * @brief 初始化主循环层。
 *
 * Steps:
 *  1. 保留主循环初始化入口，定时器事件采用直接调用。
 *
 * @return app_status_t : 操作状态。
 */
app_status_t app_loop_init(void)
{
    return APP_STATUS_OK;
}

/**
 * @brief 主循环后台轮询入口，与 do_read_message 并列调用。
 *
 * Steps:
 *  1. 执行后台运动控制逻辑（具体工程接入时填充）。
 *
 * @return app_status_t : 操作状态。
 */
app_status_t do_loop(void)
{
    //----------------------------业务填充begin----------------------------//
    // 具体工程接入时在此添加后台轮询逻辑。
    //-----------------------------业务填充end-----------------------------//
    return APP_STATUS_OK;
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
    (void)p_object;
    (void)p_data;
    //----------------------------业务填充begin----------------------------//
    // 具体工程接入时在此添加 ISR 内高频处理（如 PID，内部用 DWT 读 Δt）。
    //-----------------------------业务填充end-----------------------------//
}
