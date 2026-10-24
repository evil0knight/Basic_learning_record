/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_timer.h
 *
 * @par dependencies
 * - osal_error.h
 * - osal_types.h
 *
 * @author YA
 *
 * @brief 头文件，用于实现本模块接口及相关功能。
 *
 * Processing flow:
 *
 * 参数检查 -> 执行模块操作 -> 返回处理结果。
 *
 * @version V1.0 2026-10-24
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef OSAL_TIMER_H
#define OSAL_TIMER_H

#include "osal_error.h"
#include "osal_types.h"
/**
 * @brief 执行 osal_timer_create 操作。
 *
 * @param[in] name : 函数调用参数。
 * @param[in] period : 函数调用参数。
 * @param[in] auto_reload : 函数调用参数。
 * @param[in] callback : 函数调用参数。
 * @param[in] argument : 函数调用参数。
 * @param[in] timer : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t osal_timer_create(const char *name, osal_tick_t period,
                                bool auto_reload, osal_timer_callback_t callback,
                                void *argument, osal_timer_handle_t *timer);
/**
 * @brief 执行 osal_timer_start 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_timer_start(osal_timer_handle_t timer, osal_tick_t timeout);
/**
 * @brief 执行 osal_timer_stop 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_timer_stop(osal_timer_handle_t timer, osal_tick_t timeout);
/**
 * @brief 执行 osal_timer_reset 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_timer_reset(osal_timer_handle_t timer, osal_tick_t timeout);
/**
 * @brief 执行 osal_timer_change_period 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] new_period : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_timer_change_period(osal_timer_handle_t timer,
                                       osal_tick_t new_period,
                                       osal_tick_t timeout);
/**
 * @brief 执行 osal_timer_delete 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t osal_timer_delete(osal_timer_handle_t timer, osal_tick_t timeout);
/**
 * @brief 执行 osal_timer_get_period 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @return osal_tick_t : 函数执行结果。
 *
 */
osal_tick_t osal_timer_get_period(osal_timer_handle_t timer);

#endif
