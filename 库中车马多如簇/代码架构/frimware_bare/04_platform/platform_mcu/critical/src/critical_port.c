/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file critical_port.c
 *
 * @par dependencies
 * - critical_port.h
 * - main.h：目标工程提供的 CMSIS 设备头入口
 *
 * @author YA
 *
 * @brief 通过 CMSIS 保存和恢复 Cortex-M 中断屏蔽状态。
 *
 * Processing flow:
 *
 * 保存 PRIMASK -> 屏蔽中断 -> 恢复 PRIMASK。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "../inc/critical_port.h"

#include "main.h"
#include <stddef.h>

/**
 * @brief 保存中断状态并屏蔽可屏蔽中断，不影响 NMI 和 HardFault。
 *
 * Steps:
 *  1. 检查状态输出指针。
 *  2. 保存 PRIMASK、屏蔽中断并执行内存屏障。
 *
 * @param[out] p_irq_state : 原 PRIMASK 值。
 * @return platform_critical_status_t : 操作状态。
 */
platform_critical_status_t platform_critical_enter(
    uint32_t * const p_irq_state)
{
    //----------------------------参数校验begin----------------------------//
    if (NULL == p_irq_state)
    {
        return PLATFORM_CRITICAL_INVALID_PARAM;
    }
    //-----------------------------参数校验end-----------------------------//

    // 每次调用独立保存状态，嵌套调用不会覆盖外层状态。
    *p_irq_state = __get_PRIMASK();
    __disable_irq();
    __DMB();
    return PLATFORM_CRITICAL_OK;
}

/**
 * @brief 恢复对应 enter 保存的 PRIMASK，不强行打开中断。
 *
 * Steps:
 *  1. 检查保存值的范围。
 *  2. 完成共享存储访问并恢复中断状态。
 *
 * @param[in] u32_irq_state : 对应 enter 返回的原 PRIMASK 值。
 * @return platform_critical_status_t : 操作状态。
 */
platform_critical_status_t platform_critical_exit(uint32_t u32_irq_state)
{
    //----------------------------参数校验begin----------------------------//
    if (1U < u32_irq_state)
    {
        return PLATFORM_CRITICAL_INVALID_PARAM;
    }
    //-----------------------------参数校验end-----------------------------//

    // 先完成共享数据访问，再恢复调用前的中断屏蔽状态。
    __DMB();
    __set_PRIMASK(u32_irq_state);
    return PLATFORM_CRITICAL_OK;
}
