/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_key_handler.h
 * @par dependencies
 * - ../driver/bsp_key_driver.h
 * @author YA
 * @brief 统一管理多个按键的中断通知、消抖和长按识别。
 * Processing flow:
 * 初始化挂载 -> ISR 通知 -> 主循环扫描 -> 识别事件后调 sink。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef BSP_KEY_HANDLER_H
#define BSP_KEY_HANDLER_H

#include "../driver/bsp_key_driver.h"

typedef struct
{
    // 获取当前毫秒计数。
    en_key_status_t (*pf_get_ms)(uint32_t *p_now_ms);
    // 保存中断状态并进入临界区。
    en_key_status_t (*pf_critical_enter)(uint32_t *p_irq_state);
    // 恢复进入临界区前的中断状态。
    en_key_status_t (*pf_critical_exit)(uint32_t u32_irq_state);
    // 把中断通知回调挂到指定按键。
    en_key_status_t (*pf_irq_attach)(uint32_t u32_key_id,
                                     bsp_key_irq_callback_t pf_callback);
    // 解除指定按键的中断绑定。
    en_key_status_t (*pf_irq_detach)(uint32_t u32_key_id);
} bsp_key_runtime_ops_t;

/* 事件 sink：Handler 识别出事件后调用，Adapter 内部直接 push 到自己管的队列。 */
typedef void (*bsp_key_event_sink_t)(uint32_t u32_key_id, en_key_event_t en_event);

typedef struct
{
    bsp_key_driver_t     *p_driver;     // 连续的按键 Driver 数组。
    uint32_t            u32_key_count;  // 按键数量。
    bsp_key_runtime_ops_t st_ops;       // 共用时间、临界区及挂载接口。
    bsp_key_event_sink_t pf_sink;       // 事件 sink。
    bool                b_initialized;  // 按键组是否已挂载。
} bsp_key_handler_t;

/* 每键初始化参数：adapter 填好交给 Handler；按键 ID 等于数组下标。 */
typedef struct
{
    bool             b_active_high;     // true 表示高电平按下。
    bsp_key_config_t st_config;         // 本键的扫描/消抖/长按时序。
} bsp_key_key_init_t;

#define BSP_KEY_HANDLER_MAX_COUNT 16U

/**
 * @brief 获取唯一 Handler 对象。
 * @param[out] p_handler 共用 Handler 地址。
 * @return en_key_status_t 获取状态。
 */
en_key_status_t bsp_key_handler_get(bsp_key_handler_t **p_handler);

/**
 * @brief 挂载整组按键：内部逐键挂 Driver，再采样初始电平和挂中断。
 * @param[in,out] p_handler 共用 Handler 对象。
 * @param[in] p_port_ops Driver 使用的 GPIO 读电平接口。
 * @param[in] p_runtime_ops Handler 使用的时间/临界区/中断接口。
 * @param[in] p_a_keys 每键初始化参数数组。
 * @param[in] u32_key_count 按键数量。
 * @param[in] pf_sink 事件 sink，不能为 NULL。
 * @return en_key_status_t 挂载状态，失败时内部已完成清理。
 */
en_key_status_t bsp_key_handler_inst(bsp_key_handler_t *p_handler,
    const bsp_key_port_ops_t    *p_port_ops,
    const bsp_key_runtime_ops_t *p_runtime_ops,
    const bsp_key_key_init_t    *p_a_keys,
    uint32_t                     u32_key_count,
    bsp_key_event_sink_t         pf_sink);

/**
 * @brief ISR 仅置位对应按键的待检测标志。
 * @param[in] u32_key_id 本键 ID。
 * @return en_key_status_t 通知状态。
 */
en_key_status_t bsp_key_handler_irq_notify(uint32_t u32_key_id);

/**
 * @brief 统一扫描整组按键，识别出事件后立即调用 sink。
 * @param[in,out] p_handler 共用 Handler 对象。
 * @return en_key_status_t 首个扫描错误。
 */
en_key_status_t bsp_key_handler_scan(bsp_key_handler_t *p_handler);

/**
 * @brief 解除整组中断绑定，失败时保留对象以便重试。
 * @param[in,out] p_handler 共用 Handler 对象。
 * @return en_key_status_t 解除状态。
 */
en_key_status_t bsp_key_handler_deinit(bsp_key_handler_t *p_handler);

#endif /* BSP_KEY_HANDLER_H */
