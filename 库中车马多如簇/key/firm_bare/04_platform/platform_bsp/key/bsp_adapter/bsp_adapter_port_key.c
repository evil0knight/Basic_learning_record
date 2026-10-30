/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_adapter_port_key.c
 * @par dependencies
 * - bsp_adapter_port_key.h
 * - bsp_wrapper_key.h
 * - bsp_key_handler.h
 * - gpio_port.h
 * - critical_port.h
 * - main.h
 * @author YA
 * @brief 组装 GPIO、时间、临界区和中断接口，向 Wrapper 注册按键器件函数表。
 * Processing flow:
 * 组装函数表 -> Wrapper reg -> init 调 Handler 挂载 -> scan/get_event/notify 转发。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#include "bsp_adapter_port_key.h"

#include "../bsp_wrapper/bsp_wrapper_key.h"
#include "../../../../05_bsp/bsp_key/handler/bsp_key_handler.h"
#include "../../../../01_app/app_control/app_control.h"
#include "../../../../01_app/app_message/msg_queue.h"
#include "gpio_port.h"
#include "critical_port.h"
#include "main.h"

#include <stddef.h>

typedef struct
{
    en_core_gpio_pin_t id;              /* 逻辑 GPIO ID，在 gpio_port_config.h 登记。 */
    bool               b_active_high;   /* true 表示按下读到高电平。 */
    bsp_key_config_t   st_timing;       /* 本键扫描/消抖/长按时序。 */
} bsp_key_pin_config_t;

/* 增加表项即可扩展按键，ID 与表索引保持一致。
 * 物理 port/pin 和 EXTI pin 掩码由 platform_gpio 的 g_gpio_configs 统一登记。 */
static const bsp_key_pin_config_t g_a_key_pins[] =
{
    {CORE_GPIO_KEY0, false, {0U, 5U, 15U, 1000U}}
};

enum
{
    BSP_ADAPTER_KEY_COUNT =
        sizeof(g_a_key_pins) / sizeof(g_a_key_pins[0])
};

/* 按键 ID × 底层事件 → App 事件。新增按键时同步添加对应行。 */
static const app_event_t g_a_key_event_map[BSP_ADAPTER_KEY_COUNT][BSP_KEY_EVENT_MAX] =
{
    {EVENT_KEY0_PRESS, EVENT_KEY0_RELEASE,
    EVENT_KEY0_SHORT_PRESS, EVENT_KEY0_LONG_PRESS}
};

/* 每键的中断通知回调，由 Handler 挂进来，ISR 侧通过 pin→key_id 查到后调用。 */
static bsp_key_irq_callback_t g_a_key_irq_callback[BSP_ADAPTER_KEY_COUNT];

//======================= Port 接口（Driver 用） =======================//

/**
 * @brief 读取物理按键电平；GPIO 由 platform_gpio 的逻辑 ID 统一访问。
 */
static en_key_status_t bsp_adapter_key_read_level(uint32_t u32_key_id, bool *p_high)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_high) || (BSP_ADAPTER_KEY_COUNT <= u32_key_id))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    *p_high = (CORE_GPIO_PIN_SET ==
        core_gpio_read_pin(g_a_key_pins[u32_key_id].id));
    return BSP_KEY_OK;
}

//===================== Runtime 接口（Handler 用） =====================//

static en_key_status_t bsp_adapter_key_get_ms(uint32_t *p_now_ms)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_now_ms)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    *p_now_ms = HAL_GetTick();
    return BSP_KEY_OK;
}

static en_key_status_t bsp_adapter_key_critical_enter(uint32_t *p_irq_state)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_irq_state)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return (PLATFORM_CRITICAL_OK == platform_critical_enter(p_irq_state))
        ? BSP_KEY_OK : BSP_KEY_ERR_IO;
}

static en_key_status_t bsp_adapter_key_critical_exit(uint32_t u32_irq_state)
{
    //-------------------------参数检查 begin-------------------------//
    if (1U < u32_irq_state)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return (PLATFORM_CRITICAL_OK == platform_critical_exit(u32_irq_state))
        ? BSP_KEY_OK : BSP_KEY_ERR_IO;
}

static en_key_status_t bsp_adapter_key_irq_attach(
    uint32_t u32_key_id, bsp_key_irq_callback_t pf_callback)
{
    uint32_t u32_irq_state;

    //-------------------------参数检查 begin-------------------------//
    if ((BSP_ADAPTER_KEY_COUNT <= u32_key_id) || (NULL == pf_callback))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    if (PLATFORM_CRITICAL_OK != platform_critical_enter(&u32_irq_state))
    {
        return BSP_KEY_ERR_IO;
    }
    if (NULL != g_a_key_irq_callback[u32_key_id])
    {
        (void)platform_critical_exit(u32_irq_state);
        return BSP_KEY_ERR_BUSY;
    }
    g_a_key_irq_callback[u32_key_id] = pf_callback;
    (void)platform_critical_exit(u32_irq_state);
    return BSP_KEY_OK;
}

static en_key_status_t bsp_adapter_key_irq_detach(uint32_t u32_key_id)
{
    uint32_t u32_irq_state;

    //-------------------------参数检查 begin-------------------------//
    if (BSP_ADAPTER_KEY_COUNT <= u32_key_id)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    if (PLATFORM_CRITICAL_OK != platform_critical_enter(&u32_irq_state))
    {
        return BSP_KEY_ERR_IO;
    }
    if (NULL == g_a_key_irq_callback[u32_key_id])
    {
        (void)platform_critical_exit(u32_irq_state);
        return BSP_KEY_ERR_STATE;
    }
    g_a_key_irq_callback[u32_key_id] = NULL;
    (void)platform_critical_exit(u32_irq_state);
    return BSP_KEY_OK;
}

/* 初始化前完成组装，供 Handler 挂载时使用。 */
static const bsp_key_port_ops_t g_bsp_key_port_ops =
{
    bsp_adapter_key_read_level
};

static const bsp_key_runtime_ops_t g_bsp_key_runtime_ops =
{
    bsp_adapter_key_get_ms,
    bsp_adapter_key_critical_enter,
    bsp_adapter_key_critical_exit,
    bsp_adapter_key_irq_attach,
    bsp_adapter_key_irq_detach
};

//======================= 事件 sink（Handler 用） =======================//

/**
 * @brief Handler 识别出按键事件后调用：查映射表后 push 到 App 队列。
 * @param[in] u32_key_id 按键 ID。
 * @param[in] en_event 底层按键事件。
 */
static void bsp_adapter_key_sink(uint32_t u32_key_id, en_key_event_t en_event)
{
    //-------------------------参数检查 begin-------------------------//
    if ((BSP_ADAPTER_KEY_COUNT <= u32_key_id) ||
        ((unsigned int)en_event >= (unsigned int)BSP_KEY_EVENT_MAX))
    {
        return;
    }
    //--------------------------参数检查 end--------------------------//

    (void)queue_push(&g_msg_queue, g_a_key_event_map[u32_key_id][en_event]);
}

//========================= Wrapper 函数表 =========================//

/**
 * @brief 组装每键参数并挂载 Handler；sink 透传 App 队列。
 */
static en_key_status_t bsp_adapter_key_init(bsp_wrapper_key_dev_t *p_dev)
{
    bsp_key_handler_t  *p_handler;
    bsp_key_key_init_t  a_keys[BSP_ADAPTER_KEY_COUNT];
    uint32_t            u32_index;
    uint32_t            u32_prev;
    en_key_status_t     en_status;

    (void)p_dev;

    //-------------------------参数检查 begin-------------------------//
    for (u32_index = 0U; u32_index < BSP_ADAPTER_KEY_COUNT; u32_index++)
    {
        const bsp_key_pin_config_t *p_pin = &g_a_key_pins[u32_index];
        if (((unsigned int)p_pin->id >= (unsigned int)CORE_GPIO_PIN_MAX) ||
            (u32_index != p_pin->st_timing.u32_key_id))
        {
            return BSP_KEY_ERR_ARGUMENT;
        }
        for (u32_prev = 0U; u32_prev < u32_index; u32_prev++)
        {
            if (p_pin->id == g_a_key_pins[u32_prev].id)
            {
                return BSP_KEY_ERR_ARGUMENT;
            }
        }
    }
    //--------------------------参数检查 end--------------------------//

    /* 中断槽先清零，避免重复 init 时沿用上次残留。
     * a_keys 只是临时的入参集合，交给 Handler 完成挂载后即可丢弃。 */
    for (u32_index = 0U; u32_index < BSP_ADAPTER_KEY_COUNT; u32_index++)
    {
        g_a_key_irq_callback[u32_index] = NULL;
        a_keys[u32_index].b_active_high = g_a_key_pins[u32_index].b_active_high;
        a_keys[u32_index].st_config     = g_a_key_pins[u32_index].st_timing;
    }

    en_status = bsp_key_handler_get(&p_handler);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    return bsp_key_handler_inst(p_handler,
        &g_bsp_key_port_ops, &g_bsp_key_runtime_ops,
        a_keys, BSP_ADAPTER_KEY_COUNT, bsp_adapter_key_sink);
}

/**
 * @brief 主循环扫描：转发到 Handler，长按/短按在 Handler 内判别。
 */
static en_key_status_t bsp_adapter_key_scan(bsp_wrapper_key_dev_t *p_dev)
{
    bsp_key_handler_t *p_handler;
    en_key_status_t    en_status;

    (void)p_dev;

    en_status = bsp_key_handler_get(&p_handler);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    return bsp_key_handler_scan(p_handler);
}

/**
 * @brief ISR 中按 EXTI 引脚找到对应按键的中断回调并调用。
 *        先经 platform_gpio 把 HAL 的 pin 掩码翻译成逻辑 ID，再在 pin 表里找下标。
 */
static en_key_status_t bsp_adapter_key_notify(
    bsp_wrapper_key_dev_t *p_dev, uint16_t u16_pin)
{
    en_core_gpio_pin_t en_gpio_id;
    uint32_t           u32_index;

    (void)p_dev;

    //-------------------------参数检查 begin-------------------------//
    if ((0U == u16_pin) || (0U != (u16_pin & (u16_pin - 1U))))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    if (CORE_GPIO_OK != core_gpio_find_by_hal_pin(NULL, u16_pin, &en_gpio_id))
    {
        return BSP_KEY_ERR_STATE;
    }
    for (u32_index = 0U; u32_index < BSP_ADAPTER_KEY_COUNT; u32_index++)
    {
        if ((en_gpio_id == g_a_key_pins[u32_index].id) &&
            (NULL != g_a_key_irq_callback[u32_index]))
        {
            return g_a_key_irq_callback[u32_index](u32_index);
        }
    }
    return BSP_KEY_ERR_STATE;
}

/**
 * @brief 组装函数表并注册到 Wrapper 的 0 号槽位。
 */
en_key_status_t drv_adapter_key_register(void)
{
    static bsp_wrapper_key_dev_t dev =
    {
        .u32_idx     = 0U,
        .p_user_data = NULL,
        .init        = bsp_adapter_key_init,
        .scan        = bsp_adapter_key_scan,
        .notify      = bsp_adapter_key_notify
    };

    return bsp_wrapper_key_reg(0U, &dev) ? BSP_KEY_OK : BSP_KEY_ERR_STATE;
}
