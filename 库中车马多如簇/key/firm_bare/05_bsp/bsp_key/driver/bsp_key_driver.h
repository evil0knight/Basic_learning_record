/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_key_driver.h
 * @par dependencies
 * - ../bsp_key_types.h
 * - stdbool.h
 * @author YA
 * @brief 通过平台接口读取 GPIO 按键状态。
 * Processing flow:
 * 绑定平台读取接口 -> 将电平转换为按下状态。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef BSP_KEY_DRIVER_H
#define BSP_KEY_DRIVER_H

#include "../bsp_key_types.h"
#include <stdbool.h>

typedef struct
{
    //读取GPIO电平
    en_key_status_t (*pf_read_level)(void *p_context, bool *p_high);
} st_key_port_ops_t;

typedef struct
{
    uint32_t u32_key_id;        // 按键 ID。
    uint32_t u32_scan_ms;       // 扫描间隔，单位 ms。
    uint32_t u32_debounce_ms;   // 电平保持稳定的消抖时间，单位 ms。
    uint32_t u32_long_press_ms; // 确认按下后触发长按的时间，单位 ms。
} st_key_config_t;

typedef struct st_key_driver
{
    st_key_port_ops_t st_port_ops;    // GPIO 读取接口。
    void            *p_port_context; // 包含 GPIO 端口和引脚的平台对象。
    bool             b_active_high;  // true 高电平按下，false 低电平按下。
    bool             b_initialized;  // 接口是否已挂载。

    // 读取按键是否按下。
    en_key_status_t (*pf_read_pressed)(
        const struct st_key_driver *p_driver, bool *p_pressed);
    st_key_config_t st_config;             // 本键 ID 和时间配置。
    volatile bool  b_pending;              // ISR 置位的待检测标志。
    bool           b_active;               // 本键是否在检测中。
    bool           b_candidate_pressed;    // 消抖中的候选状态。
    bool           b_stable_pressed;       // 消抖确认后的状态。
    bool           b_long_reported;        // 本次是否已记录长按。
    bool           b_irq_attached;         // 本键中断对象是否已挂载。
    uint32_t       u32_last_scan_ms;        // 上次采样时间，单位 ms。
    uint32_t       u32_candidate_since_ms;  // 候选状态开始时间，单位 ms。
    uint32_t       u32_press_since_ms;      // 确认按下时间，单位 ms。
    uint32_t       u32_event_flags;         // 待取走的动作位。
} st_key_driver_t;

/**
 * @brief 初始化零初始化的 Driver 对象并复制平台操作表。
 * @param[in,out] p_driver Driver 对象。
 * @param[in] p_ops 平台读取接口。
 * @param[in] p_context 平台上下文，允许为空。
 * @param[in] b_active_high 为 true 时高电平按下。
 * @return en_key_status_t 初始化结果。
 */
en_key_status_t bsp_key_driver_init(st_key_driver_t *p_driver,
    const st_key_port_ops_t *p_ops, void *p_context, bool b_active_high);

/**
 * @brief 读取按键是否按下，读取失败时不修改输出。
 * @param[in] p_driver 已初始化的 Driver。
 * @param[out] p_pressed 按下状态。
 * @return en_key_status_t 读取结果。
 */
en_key_status_t bsp_key_driver_read_pressed(
    const st_key_driver_t *p_driver, bool *p_pressed);

/**
 * @brief 清除 Driver 的平台绑定，须先解除 Handler 挂载。
 * @param[in,out] p_driver Driver 对象。
 * @return en_key_status_t 释放结果。
 */
en_key_status_t bsp_key_driver_deinit(st_key_driver_t *p_driver);

#endif /* BSP_KEY_DRIVER_H */
