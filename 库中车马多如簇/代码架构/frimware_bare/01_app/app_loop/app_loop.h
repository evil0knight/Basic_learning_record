/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file app_loop.h
 *
 * @par dependencies
 * - ../app_control/app_control.h
 *
 * @author YA
 *
 * @brief 主循环后台轮询与定时器 ISR 回调接口。
 *
 * Processing flow:
 *
 * app_loop_init() 挂载 pf_tim_interrupt_callback -> do_loop() 主循环轮询 ->
 * do_loop_tick() 定时器 ISR 内执行。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef APP_LOOP_H
#define APP_LOOP_H

#include "../app_control/app_control.h"
/**
 * @brief 执行 app_loop_init 操作。
 *
 * @return app_status_t : 函数执行结果。
 *
 */
app_status_t app_loop_init(void);
/**
 * @brief 执行 do_loop 操作。
 *
 * @return app_status_t : 函数执行结果。
 *
 */
app_status_t do_loop(void);
/**
 * @brief 执行 do_loop_tick 操作。
 *
 * @param[in] p_object : 函数调用参数。
 * @param[in] p_data : 函数调用参数。
 *
 */
void do_loop_tick(void *p_object, void *p_data);

#endif /* APP_LOOP_H */
