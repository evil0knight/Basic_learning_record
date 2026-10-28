/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_wrapper_key.h
 * @par dependencies
 * - ../../../05_bsp/bsp_key/bsp_key_types.h
 * @author YA
 * @brief 提供按键挂载、扫描和事件读取入口。
 * Processing flow:
 * 注册已绑定的设备描述 -> 主循环调用 scan。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef BSP_WRAPPER_KEY_H
#define BSP_WRAPPER_KEY_H

#include "../../../05_bsp/bsp_key/bsp_key_types.h"

/* 按键设备描述，由 Adapter 组装、Wrapper 保存。 */
typedef struct st_key_drv
{
    uint32_t u32_idx;       // 设备槽位索引
    uint32_t u32_dev_id;    // 设备标识
    void    *p_user_data;   // Adapter 平台对象数组地址

    /* 扫描按键，更新消抖、长按状态。 */
    en_key_status_t (*pf_key_drv_scan)(struct st_key_drv *p_dev);

    /* 解除中断绑定并清除检测状态。 */
    en_key_status_t (*pf_key_drv_deinit)(struct st_key_drv *p_dev);

    /* 中断回调函数指针 */
    en_key_status_t (*pf_key_drv_take_events)(struct st_key_drv *p_dev,
        uint32_t u32_key_id, uint32_t *p_flags);

    /* 按 EXTI 引脚通知对应按键。 */
    en_key_status_t (*pf_key_drv_irq_notify)(struct st_key_drv *p_dev,
        uint16_t u16_pin);
} st_key_drv_t;

/**
 * @brief 将设备描述复制到 Wrapper，注册项成为当前设备。
 * @param[in] u32_index Wrapper 设备槽位，范围 0～1。
 * @param[in] p_dev 设备描述，user_data 指向 Adapter 平台对象数组。
 * @return en_key_status_t 注册结果。
 */
en_key_status_t bsp_wrapper_key_reg(
    uint32_t u32_index, const st_key_drv_t *p_dev);

/**
 * @brief 主循环按键扫描入口，不等待时间到期。
 * @param[in] 无；检查设备挂载状态。
 * @return en_key_status_t 扫描结果。
 */
en_key_status_t bsp_wrapper_key_scan(void);

/**
 * @brief HAL EXTI 回调中转发引脚，内部查找已挂载的 Handler。
 * @param[in] u16_pin 单一 EXTI 引脚掩码。
 * @return en_key_status_t 通知结果，未挂载时返回状态错误。
 */
en_key_status_t bsp_wrapper_key_irq_notify(uint16_t u16_pin);

/**
 * @brief 解除全部按键的中断绑定。
 * @param[in] 无；检查设备挂载状态。
 * @return en_key_status_t 释放结果，失败时允许再次释放。
 */
en_key_status_t bsp_wrapper_key_deinit(void);

/**
 * @brief 主循环取出并清除指定按键的事件位。
 * @param[in] u32_key_id 按键 ID。
 * @param[out] p_flags 动作位集合，位号对应 en_key_event_t。
 * @return en_key_status_t 查询结果。
 */
en_key_status_t bsp_wrapper_key_take_events(
    uint32_t u32_key_id, uint32_t *p_flags);

#endif /* BSP_WRAPPER_KEY_H */
