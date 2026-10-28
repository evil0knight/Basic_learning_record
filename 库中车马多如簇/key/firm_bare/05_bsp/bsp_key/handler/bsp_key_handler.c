/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_key_handler.c
 * @par dependencies
 * - bsp_key_handler.h
 * - stddef.h
 * @author YA
 * @brief 实现双向消抖、短按及长按一次触发。
 * Processing flow:
 * 取走中断通知 -> 定时采样 -> 稳定状态迁移 -> 保存事件位。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#include "bsp_key_handler.h"
#include <stddef.h>

/* 一个 Handler 管理整组按键，每键状态由 Driver 独立保存。 */
static st_key_driver_t  g_a_key_driver[BSP_KEY_HANDLER_MAX_COUNT];
static st_key_handler_t g_st_key_handler;

/**
 * @brief 获取指定按键的静态对象，不改变对象状态。
 * @param[in] u32_index 对象索引。
 * @param[out] p_driver Driver 对象地址。
 * @param[out] p_handler Handler 对象地址。
 * @return en_key_status_t 获取结果。
 */
en_key_status_t bsp_key_handler_get_objects(uint32_t u32_index,
    st_key_driver_t **p_driver, st_key_handler_t **p_handler)
{
    //-------------------------参数检查 begin-------------------------//
    if ((BSP_KEY_HANDLER_MAX_COUNT <= u32_index) ||
        (NULL == p_driver) || (NULL == p_handler))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    *p_driver  = &g_a_key_driver[u32_index];
    *p_handler = &g_st_key_handler;
    return BSP_KEY_OK;
}


/**
 * @brief 设置对应的按键事件位。
 * @param[in,out] p_driver 本键 Driver 对象。
 * @param[in] en_event 按键动作。
 * @return en_key_status_t 保存状态。
 */
static en_key_status_t bsp_key_handler_record_event(
    st_key_driver_t *p_driver, en_key_event_t en_event)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_driver) ||
        ((unsigned int)en_event >= (unsigned int)BSP_KEY_EVENT_MAX))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if (!p_driver->b_initialized)
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    p_driver->u32_event_flags |= (UINT32_C(1) << (uint32_t)en_event);
    return BSP_KEY_OK;
}

/**
 * @brief 使用本轮共用时间处理一个按键。
 * @param[in,out] p_driver 本键对象。
 * @param[in] u32_now_ms 本轮时间戳。
 * @return en_key_status_t 扫描状态。
 */
static en_key_status_t bsp_key_handler_scan_one(
    st_key_driver_t *p_driver, uint32_t u32_now_ms)
{
    bool b_pressed;
    en_key_status_t en_status;
    en_key_status_t en_result = BSP_KEY_OK;

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
            // 确认按下，开始长按计时并记录按下事件。
            p_driver->u32_press_since_ms = u32_now_ms;
            p_driver->b_long_reported = false;
            en_result = bsp_key_handler_record_event(
                p_driver, BSP_KEY_EVENT_PRESS);
        }
        else
        {
            // 确认松开，结束检测并记录松开事件。
            p_driver->b_active = false;
            en_result = bsp_key_handler_record_event(
                p_driver, BSP_KEY_EVENT_RELEASE);
            if (!p_driver->b_long_reported)
            {
                // 未触发长按，本次记为短按。
                en_status = bsp_key_handler_record_event(
                    p_driver, BSP_KEY_EVENT_SHORT_PRESS);
                if (BSP_KEY_OK == en_result)
                {
                    en_result = en_status;
                }
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
        en_status = bsp_key_handler_record_event(
            p_driver, BSP_KEY_EVENT_LONG_PRESS);
        if (BSP_KEY_OK == en_result)
        {
            en_result = en_status;
        }
    }
    if ((!p_driver->b_stable_pressed) && (!b_pressed))
    {
        p_driver->b_active = false;
    }
    return en_result;
}

/**
 * @brief 挂载整组接口及中断对象，使用同一个初始时间戳。
 * @param[in,out] p_handler 共用 Handler。
 * @param[in] p_driver Driver 数组。
 * @param[in] u32_key_count 按键数量。
 * @param[in] p_ops 共用运行接口。
 * @param[in] p_context 时间接口上下文。
 * @return en_key_status_t 挂载状态。
 */
en_key_status_t bsp_key_handler_init(st_key_handler_t *p_handler,
    st_key_driver_t *p_driver, uint32_t u32_key_count,
    const st_key_runtime_ops_t *p_ops, void *p_context)
{
    uint32_t u32_index;
    uint32_t u32_now_ms;
    bool b_pressed;
    en_key_status_t en_status;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_handler) || (NULL == p_driver) || (NULL == p_ops) ||
        (0U == u32_key_count) || (BSP_KEY_HANDLER_MAX_COUNT < u32_key_count))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if ((NULL == p_ops->pf_get_ms) ||
        (NULL == p_ops->pf_critical_enter) ||
        (NULL == p_ops->pf_critical_exit) ||
        (NULL == p_ops->pf_irq_attach) || (NULL == p_ops->pf_irq_detach))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if (p_handler->b_initialized)
    {
        return BSP_KEY_ERR_STATE;
    }
    for (u32_index = 0U; u32_index < u32_key_count; u32_index++)
    {
        const st_key_driver_t *p_key = &p_driver[u32_index];
        const st_key_config_t *p_config = &p_key->st_config;
        if ((!p_key->b_initialized) || p_key->b_irq_attached ||
            (NULL == p_key->pf_read_pressed))
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

    en_status = p_ops->pf_get_ms(p_context, &u32_now_ms);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    p_handler->p_driver      = p_driver;
    p_handler->u32_key_count = u32_key_count;
    p_handler->st_ops        = *p_ops;
    p_handler->p_context     = p_context;
    p_handler->b_initialized = true;
    for (u32_index = 0U; u32_index < u32_key_count; u32_index++)
    {
        st_key_driver_t *p_key = &p_driver[u32_index];
        en_status = p_key->pf_read_pressed(p_key, &b_pressed);
        if (BSP_KEY_OK != en_status)
        {
            return en_status;
        }
        p_key->b_active               = b_pressed;
        p_key->b_candidate_pressed    = b_pressed;
        p_key->u32_candidate_since_ms = u32_now_ms;
        p_key->u32_last_scan_ms       = u32_now_ms;
        en_status = p_ops->pf_irq_attach(p_key->p_port_context,
            bsp_key_handler_irq_notify, p_key);
        if (BSP_KEY_OK != en_status)
        {
            return en_status;
        }
        p_key->b_irq_attached = true;
        p_key->b_pending = true; // 首轮采样覆盖挂载期间的电平变化。
    }
    return BSP_KEY_OK;
}

/**
 * @brief ISR 通知对应按键开始检测，不改动消抖和长按计时。
 * @param[in,out] p_object 本键 Driver 对象。
 * @return en_key_status_t 通知状态。
 */
en_key_status_t bsp_key_handler_irq_notify(void *p_object)
{
    st_key_driver_t *p_driver = p_object;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_driver)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if (!p_driver->b_initialized)
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    p_driver->b_pending = true;
    return BSP_KEY_OK;
}

/**
 * @brief 批量取走中断通知，一次读取时间后遍历活动键。
 * @param[in,out] p_handler 共用 Handler。
 * @return en_key_status_t 首个扫描错误。
 */
en_key_status_t bsp_key_handler_scan(st_key_handler_t *p_handler)
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
        const st_key_driver_t *p_key = &p_handler->p_driver[u32_index];
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
        st_key_driver_t *p_key = &p_handler->p_driver[u32_index];
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
    en_status = p_handler->st_ops.pf_get_ms(
        p_handler->p_context, &u32_now_ms);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    for (u32_index = 0U; u32_index < p_handler->u32_key_count; u32_index++)
    {
        st_key_driver_t *p_key = &p_handler->p_driver[u32_index];
        if (!p_key->b_active)
        {
            continue;
        }
        en_status = bsp_key_handler_scan_one(p_key, u32_now_ms);
        if (BSP_KEY_OK == en_result)
        {
            en_result = en_status;
        }
    }
    return en_result;
}

/**
 * @brief 主循环取走指定按键的动作位。
 * @param[in,out] p_handler 共用 Handler。
 * @param[in] u32_key_id 按键 ID。
 * @param[out] p_flags 读取后清零的动作位。
 * @return en_key_status_t 读取状态。
 */
en_key_status_t bsp_key_handler_take_events(st_key_handler_t *p_handler,
    uint32_t u32_key_id, uint32_t *p_flags)
{
    st_key_driver_t *p_key;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_handler) || (NULL == p_flags))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if (!p_handler->b_initialized)
    {
        return BSP_KEY_ERR_STATE;
    }
    if (p_handler->u32_key_count <= u32_key_id)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    p_key = &p_handler->p_driver[u32_key_id];
    *p_flags = p_key->u32_event_flags;
    p_key->u32_event_flags = 0U;
    return BSP_KEY_OK;
}

/**
 * @brief 逆序解除各键中断，全部成功后释放共用 Handler。
 * @param[in,out] p_handler 共用 Handler。
 * @return en_key_status_t 首个解除错误。
 */
en_key_status_t bsp_key_handler_deinit(st_key_handler_t *p_handler)
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
        st_key_driver_t *p_key = &p_handler->p_driver[u32_index - 1U];
        if (!p_key->b_irq_attached)
        {
            continue;
        }
        en_status = p_handler->st_ops.pf_irq_detach(
            p_key->p_port_context, p_key);
        if (BSP_KEY_OK == en_status)
        {
            p_key->b_irq_attached = false;
        }
        else if (BSP_KEY_OK == en_result)
        {
            en_result = en_status;
        }
    }
    if (BSP_KEY_OK == en_result)
    {
        *p_handler = (st_key_handler_t){0};
    }
    return en_result;
}
