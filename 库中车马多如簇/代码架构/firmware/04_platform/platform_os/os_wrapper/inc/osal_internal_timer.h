/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file osal_internal_timer.h
 *
 * @par dependencies
 * - osal_config.h
 * - osal_timer.h
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

#ifndef OSAL_INTERNAL_TIMER_H
#define OSAL_INTERNAL_TIMER_H

#include "osal_config.h"
#include "osal_timer.h"

typedef struct
{
    char name[OSAL_NAME_MAX];
    osal_tick_t period;
    bool auto_reload;
    osal_timer_callback_t callback;
    void *argument;
    void *native_timer;
} osal_timer_record_t;
/**
 * @brief 执行 os_timer_create_impl 操作。
 *
 * @param[in] record : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */

osal_status_t os_timer_create_impl(osal_timer_record_t *record);
/**
 * @brief 执行 os_timer_start_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_timer_start_impl(osal_timer_handle_t timer,
                                  osal_tick_t timeout);
/**
 * @brief 执行 os_timer_stop_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_timer_stop_impl(osal_timer_handle_t timer,
                                 osal_tick_t timeout);
/**
 * @brief 执行 os_timer_reset_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_timer_reset_impl(osal_timer_handle_t timer,
                                  osal_tick_t timeout);
/**
 * @brief 执行 os_timer_change_period_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] new_period : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_timer_change_period_impl(osal_timer_handle_t timer,
                                          osal_tick_t new_period,
                                          osal_tick_t timeout);
/**
 * @brief 执行 os_timer_delete_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @param[in] timeout : 函数调用参数。
 * @return osal_status_t : 函数执行结果。
 *
 */
osal_status_t os_timer_delete_impl(osal_timer_handle_t timer,
                                   osal_tick_t timeout);
/**
 * @brief 执行 os_timer_get_period_impl 操作。
 *
 * @param[in] timer : 函数调用参数。
 * @return osal_tick_t : 函数执行结果。
 *
 */
osal_tick_t os_timer_get_period_impl(osal_timer_handle_t timer);

#endif
