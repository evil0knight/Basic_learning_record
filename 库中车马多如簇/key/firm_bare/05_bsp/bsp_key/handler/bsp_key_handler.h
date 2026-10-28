/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_key_handler.h
 * @par dependencies
 * - ../driver/bsp_key_driver.h
 * @author YA
 * @brief 统一管理多个按键的中断通知、消抖和长按识别。
 * Processing flow:
 * 初始化挂载 -> ISR 通知 -> 主循环扫描 -> 保存事件位 -> 主循环取走。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef BSP_KEY_HANDLER_H
#define BSP_KEY_HANDLER_H

#include "../driver/bsp_key_driver.h"

typedef struct
{
    // 获取当前毫秒计数
    en_key_status_t (*pf_get_ms)(void *p_context, uint32_t *p_now_ms);
    // 保存中断状态并进入临界区
    en_key_status_t (*pf_critical_enter)(uint32_t *p_irq_state);
    // 恢复进入临界区前的中断状态
    en_key_status_t (*pf_critical_exit)(uint32_t u32_irq_state);
    // 将中断通知回调和 Driver 对象绑定到对应按键。
    en_key_status_t (*pf_irq_attach)(void *p_context,
                                     pf_key_irq_callback_t pf_callback,
                                     void *p_object);
    // 解除对应按键与 Driver 对象的中断绑定。
    en_key_status_t (*pf_irq_detach)(void *p_context, void *p_object);
} st_key_runtime_ops_t;

typedef struct
{
    st_key_driver_t     *p_driver;       // 连续的按键 Driver 数组。
    uint32_t            u32_key_count;  // 按键数量。
    st_key_runtime_ops_t st_ops;         // 共用时间、临界区及挂载接口。
    void               *p_context;      // 共用时间接口上下文。
    bool                b_initialized;  // 按键组是否已挂载。
} st_key_handler_t;

#define BSP_KEY_HANDLER_MAX_COUNT 16U

/**
 * @brief 获取静态 Driver 和唯一 Handler 对象。
 * @param[in] u32_index Driver 索引，范围 0～15。
 * @param[out] p_driver 本键 Driver 地址。
 * @param[out] p_handler 共用 Handler 地址。
 * @return en_key_status_t 获取状态。
 */
en_key_status_t bsp_key_handler_get_objects(uint32_t u32_index,
    st_key_driver_t **p_driver, st_key_handler_t **p_handler);

/**
 * @brief 挂载整组按键，一次读取时间并采样各键初始状态。
 * @param[in,out] p_handler 共用 Handler 对象。
 * @param[in] p_driver 已绑定接口及配置的 Driver 数组。
 * @param[in] u32_key_count 按键数量。
 * @param[in] p_ops 共用运行接口。
 * @param[in] p_context 时间接口上下文。
 * @return en_key_status_t 挂载状态，失败后须调用 deinit 清理。
 */
en_key_status_t bsp_key_handler_init(st_key_handler_t *p_handler,
    st_key_driver_t *p_driver, uint32_t u32_key_count,
    const st_key_runtime_ops_t *p_ops, void *p_context);

/**
 * @brief ISR 仅置位对应 Driver 的待检测标志。
 * @param[in,out] p_object 本键 Driver 对象。
 * @return en_key_status_t 通知状态。
 */
en_key_status_t bsp_key_handler_irq_notify(void *p_object);

/**
 * @brief 统一扫描整组按键，有活动键时本轮只读取一次时间。
 * @param[in,out] p_handler 共用 Handler 对象。
 * @return en_key_status_t 首个扫描错误。
 */
en_key_status_t bsp_key_handler_scan(st_key_handler_t *p_handler);

/**
 * @brief 取出并清除指定按键的事件位，仅由主循环调用。
 * @param[in,out] p_handler 共用 Handler 对象。
 * @param[in] u32_key_id 按键 ID，与 Driver 数组索引一致。
 * @param[out] p_flags 动作位，0 表示无事件。
 * @return en_key_status_t 读取状态。
 */
en_key_status_t bsp_key_handler_take_events(st_key_handler_t *p_handler,
    uint32_t u32_key_id, uint32_t *p_flags);

/**
 * @brief 解除整组中断绑定，失败时保留对象以便重试。
 * @param[in,out] p_handler 共用 Handler 对象。
 * @return en_key_status_t 解除状态。
 */
en_key_status_t bsp_key_handler_deinit(st_key_handler_t *p_handler);

#endif /* BSP_KEY_HANDLER_H */
