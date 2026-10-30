/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_key_handler.c
 * @par dependencies
 * - bsp_key_handler.h
 * - stddef.h
 * @author YA
 * @brief 实现双向消抖、短按及长按一次触发；事件识别后直接 push 给 Adapter。
 * Processing flow:
 * 取走中断通知 -> 定时采样 -> 稳定状态迁移 -> 调 sink 推送事件。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#include "bsp_key_handler.h"
#include <stddef.h>

/* 一个 Handler 管理整组按键，每键状态由 Driver 独立保存。 */
static bsp_key_driver_t  g_a_key_driver[BSP_KEY_HANDLER_MAX_COUNT];
static bsp_key_handler_t g_bsp_key_handler;

/**
 * @brief 获取唯一的 Handler 对象，不改变其状态。
 */
en_key_status_t bsp_key_handler_get(bsp_key_handler_t **p_handler)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handler)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    *p_handler = &g_bsp_key_handler;
    return BSP_KEY_OK;
}

/**
 * @brief 通过 sink 推送一个按键事件。
 */
static void bsp_key_handler_emit_event(
    bsp_key_handler_t *p_handler,
    uint32_t u32_key_id, en_key_event_t en_event)
{
    if (NULL != p_handler->pf_sink)
    {
        p_handler->pf_sink(u32_key_id, en_event);
    }
}

/**
 * @brief 使用本轮共用时间处理一个按键。
 * @param[in,out] p_handler 共用 Handler，用于访问 sink。
 * @param[in,out] p_driver 本键对象。
 * @param[in] u32_key_id 本键 ID（即数组下标）。
 * @param[in] u32_now_ms 本轮时间戳。
 */
static en_key_status_t bsp_key_handler_scan_one(
    bsp_key_handler_t *p_handler,
    bsp_key_driver_t *p_driver,
    uint32_t u32_key_id,
    uint32_t u32_now_ms)
{
    bool b_pressed;
    en_key_status_t en_status;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) || (NULL == p_driver->pf_read_pressed))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    if ((uint32_t)(u32_now_ms - p_driver->u32_last_scan_ms) <
        p_driver->st_config.u32_scan_ms)
    {
        return BSP_KEY_OK;
    }

    // 到达采样间隔，读取按键。
    en_status = p_driver->pf_read_pressed(p_driver, &b_pressed);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }

    p_driver->u32_last_scan_ms = u32_now_ms;
    // 电平变化，重新开始消抖计时。
    if (b_pressed != p_driver->b_candidate_pressed)
    {
        p_driver->b_candidate_pressed = b_pressed;
        p_driver->u32_candidate_since_ms = u32_now_ms;
    }
    // 新状态保持稳定达到消抖时间后，确认按下或松开。
    if ((b_pressed != p_driver->b_stable_pressed) &&
        ((uint32_t)(u32_now_ms - p_driver->u32_candidate_since_ms) >=
         p_driver->st_config.u32_debounce_ms))
    {
        p_driver->b_stable_pressed = b_pressed;
        if (b_pressed)
        {
            // 确认按下，开始长按计时并发送按下事件。
            p_driver->u32_press_since_ms = u32_now_ms;
            p_driver->b_long_reported = false;
            bsp_key_handler_emit_event(p_handler, u32_key_id, BSP_KEY_EVENT_PRESS);
        }
        else
        {
            // 确认松开，发送松开事件；未触发长按时追加短按。
            p_driver->b_active = false;
            bsp_key_handler_emit_event(p_handler, u32_key_id, BSP_KEY_EVENT_RELEASE);
            if (!p_driver->b_long_reported)
            {
                bsp_key_handler_emit_event(p_handler, u32_key_id, BSP_KEY_EVENT_SHORT_PRESS);
            }
        }
    }

    /* 松开消抖期间不产生长按；每次按住只记录一次长按。 */
    if (p_driver->b_stable_pressed && b_pressed &&
        (!p_driver->b_long_reported) &&
        ((uint32_t)(u32_now_ms - p_driver->u32_press_since_ms) >=
         p_driver->st_config.u32_long_press_ms))
    {
        p_driver->b_long_reported = true;
        bsp_key_handler_emit_event(p_handler, u32_key_id, BSP_KEY_EVENT_LONG_PRESS);
    }
    if ((!p_driver->b_stable_pressed) && (!b_pressed))
    {
        p_driver->b_active = false;
    }
    return BSP_KEY_OK;
}

/**
 * @brief 挂载整组按键：内部逐键挂 Driver，再采样初始电平和挂中断。
 */
en_key_status_t bsp_key_handler_inst(bsp_key_handler_t *p_handler,
    const bsp_key_port_ops_t    *p_port_ops,
    const bsp_key_runtime_ops_t *p_runtime_ops,
    const bsp_key_key_init_t    *p_a_keys,
    uint32_t                     u32_key_count,
    bsp_key_event_sink_t         pf_sink)
{
    uint32_t u32_index;
    uint32_t u32_now_ms;
    bool b_pressed;
    en_key_status_t en_status;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_handler) || (NULL == p_port_ops) ||
        (NULL == p_runtime_ops) || (NULL == p_a_keys) ||
        (NULL == pf_sink) ||
        (0U == u32_key_count) ||
        (BSP_KEY_HANDLER_MAX_COUNT < u32_key_count))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if ((NULL == p_port_ops->pf_read_level) ||
        (NULL == p_runtime_ops->pf_get_ms) ||
        (NULL == p_runtime_ops->pf_critical_enter) ||
        (NULL == p_runtime_ops->pf_critical_exit) ||
        (NULL == p_runtime_ops->pf_irq_attach) ||
        (NULL == p_runtime_ops->pf_irq_detach))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if (p_handler->b_initialized)
    {
        return BSP_KEY_ERR_STATE;
    }
    for (u32_index = 0U; u32_index < u32_key_count; u32_index++)
    {
        const bsp_key_config_t *p_config = &p_a_keys[u32_index].st_config;
        if (g_a_key_driver[u32_index].b_initialized)
        {
            return BSP_KEY_ERR_STATE;
        }
        if ((u32_index != p_config->u32_key_id) ||
            (0U == p_config->u32_scan_ms) ||
            (0U == p_config->u32_debounce_ms) ||
            (0U == p_config->u32_long_press_ms) ||
            (INT32_MAX < p_config->u32_scan_ms) ||
            (INT32_MAX < p_config->u32_debounce_ms) ||
            (INT32_MAX < p_config->u32_long_press_ms))
        {
            return BSP_KEY_ERR_ARGUMENT;
        }
    }
    //--------------------------参数检查 end--------------------------//

    /* 逐键挂 Driver 并写入配置，失败时逆序释放已挂的 Driver。 */
    for (u32_index = 0U; u32_index < u32_key_count; u32_index++)
    {
        bsp_key_driver_t *p_key = &g_a_key_driver[u32_index];
        en_status = bsp_key_driver_inst(p_key, p_port_ops,
            u32_index,
            p_a_keys[u32_index].b_active_high);
        if (BSP_KEY_OK != en_status)
        {
            while (0U < u32_index)
            {
                u32_index--;
                (void)bsp_key_driver_deinit(&g_a_key_driver[u32_index]);
            }
            return en_status;
        }
        p_key->st_config = p_a_keys[u32_index].st_config;
    }

    /* 读取共用初始时间戳，失败则回滚 Driver。 */
    en_status = p_runtime_ops->pf_get_ms(&u32_now_ms);
    if (BSP_KEY_OK != en_status)
    {
        for (u32_index = u32_key_count; 0U < u32_index; u32_index--)
        {
            (void)bsp_key_driver_deinit(&g_a_key_driver[u32_index - 1U]);
        }
        return en_status;
    }
    p_handler->p_driver      = g_a_key_driver;
    p_handler->u32_key_count = u32_key_count;
    p_handler->st_ops        = *p_runtime_ops;
    p_handler->pf_sink       = pf_sink;
    p_handler->b_initialized = true;

    /* 采样初始电平并挂中断，首轮采样覆盖挂载期间的电平变化。 */
    for (u32_index = 0U; u32_index < u32_key_count; u32_index++)
    {
        bsp_key_driver_t *p_key = &g_a_key_driver[u32_index];
        en_status = p_key->pf_read_pressed(p_key, &b_pressed);
        if (BSP_KEY_OK != en_status)
        {
            (void)bsp_key_handler_deinit(p_handler);
            return en_status;
        }
        p_key->b_active               = b_pressed;
        p_key->b_candidate_pressed    = b_pressed;
        p_key->u32_candidate_since_ms = u32_now_ms;
        p_key->u32_last_scan_ms       = u32_now_ms;
        en_status = p_runtime_ops->pf_irq_attach(u32_index,
            bsp_key_handler_irq_notify);
        if (BSP_KEY_OK != en_status)
        {
            (void)bsp_key_handler_deinit(p_handler);
            return en_status;
        }
        p_key->b_irq_attached = true;
        p_key->b_pending      = true;
    }
    return BSP_KEY_OK;
}

/**
 * @brief ISR 通知对应按键开始检测，不改动消抖和长按计时。
 */
en_key_status_t bsp_key_handler_irq_notify(uint32_t u32_key_id)
{
    //-------------------------参数检查 begin-------------------------//
    if ((!g_bsp_key_handler.b_initialized) ||
        (g_bsp_key_handler.u32_key_count <= u32_key_id))
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    g_a_key_driver[u32_key_id].b_pending = true;
    return BSP_KEY_OK;
}

/**
 * @brief 批量取走中断通知，一次读取时间后遍历活动键。
 */
en_key_status_t bsp_key_handler_scan(bsp_key_handler_t *p_handler)
{
    uint32_t u32_index;
    uint32_t u32_irq_state;
    uint32_t u32_now_ms;
    bool b_work = false;
    en_key_status_t en_status;
    en_key_status_t en_result = BSP_KEY_OK;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handler)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if (!p_handler->b_initialized)
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    for (u32_index = 0U; u32_index < p_handler->u32_key_count; u32_index++)
    {
        const bsp_key_driver_t *p_key = &p_handler->p_driver[u32_index];
        if (p_key->b_active || p_key->b_pending)
        {
            b_work = true;
            break;
        }
    }
    // 全部空闲，不进入临界区，也不读取时间或 GPIO。
    if (!b_work)
    {
        return BSP_KEY_OK;
    }
    en_status = p_handler->st_ops.pf_critical_enter(&u32_irq_state);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    for (u32_index = 0U; u32_index < p_handler->u32_key_count; u32_index++)
    {
        bsp_key_driver_t *p_key = &p_handler->p_driver[u32_index];
        if (p_key->b_pending)
        {
            p_key->b_active  = true;
            p_key->b_pending = false;
        }
    }
    en_status = p_handler->st_ops.pf_critical_exit(u32_irq_state);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    // 本轮所有按键共用此时间戳。
    en_status = p_handler->st_ops.pf_get_ms(&u32_now_ms);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    for (u32_index = 0U; u32_index < p_handler->u32_key_count; u32_index++)
    {
        bsp_key_driver_t *p_key = &p_handler->p_driver[u32_index];
        if (!p_key->b_active)
        {
            continue;
        }
        en_status = bsp_key_handler_scan_one(p_handler, p_key, u32_index, u32_now_ms);
        if (BSP_KEY_OK == en_result)
        {
            en_result = en_status;
        }
    }
    return en_result;
}

/**
 * @brief 逆序解除各键中断并释放对应 Driver。
 */
en_key_status_t bsp_key_handler_deinit(bsp_key_handler_t *p_handler)
{
    uint32_t u32_index;
    en_key_status_t en_status;
    en_key_status_t en_result = BSP_KEY_OK;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handler)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if (!p_handler->b_initialized)
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    for (u32_index = p_handler->u32_key_count; 0U < u32_index; u32_index--)
    {
        bsp_key_driver_t *p_key = &p_handler->p_driver[u32_index - 1U];
        if (p_key->b_irq_attached)
        {
            en_status = p_handler->st_ops.pf_irq_detach(p_key->u32_key_id);
            if (BSP_KEY_OK != en_status)
            {
                if (BSP_KEY_OK == en_result)
                {
                    en_result = en_status;
                }
                continue; // 中断未解，保留 Driver。
            }
            p_key->b_irq_attached = false;
        }
        (void)bsp_key_driver_deinit(p_key);
    }
    if (BSP_KEY_OK == en_result)
    {
        *p_handler = (bsp_key_handler_t){0};
    }
    return en_result;
}
