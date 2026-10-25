/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file critical_port.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief 单核 Cortex-M 裸机临界区接口。
 *
 * Processing flow:
 *
 * 保存 PRIMASK 并屏蔽中断 -> 操作共享数据 -> 恢复 PRIMASK。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef CRITICAL_PORT_H
#define CRITICAL_PORT_H

#include <stdint.h>

typedef enum
{
    PLATFORM_CRITICAL_OK = 0,
    PLATFORM_CRITICAL_INVALID_PARAM
} platform_critical_status_t;

/**
 * @brief 保存中断屏蔽状态并进入临界区。
 *
 * Steps:
 *  1. 检查输出指针。
 *  2. 保存 PRIMASK 并屏蔽可屏蔽中断。
 *
 * @param[out] p_irq_state : 本次调用独占的原中断状态存储。
 * @return platform_critical_status_t : 操作状态。
 */
platform_critical_status_t platform_critical_enter(
    uint32_t * const p_irq_state);

/**
 * @brief 恢复进入临界区前的中断状态，支持按逆序退出嵌套。
 *
 * Steps:
 *  1. 检查保存的 PRIMASK 值。
 *  2. 完成共享存储访问并恢复原状态。
 *
 * @param[in] u32_irq_state : 对应 enter 保存的状态，必须成对使用。
 * @return platform_critical_status_t : 操作状态。
 */
platform_critical_status_t platform_critical_exit(uint32_t u32_irq_state);

#endif /* CRITICAL_PORT_H */
