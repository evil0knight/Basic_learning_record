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
 * @brief 组装 GPIO、时间、临界区和中断挂载接口。
 * Processing flow:
 * 静态配置 -> 绑定接口及中断对象 -> Wrapper 扫描。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#include "bsp_adapter_port_key.h"

#include "../bsp_wrapper/bsp_wrapper_key.h"
#include "../../../05_bsp/bsp_key/handler/bsp_key_handler.h"
#include "gpio_port.h"
#include "critical_port.h"
#include "main.h"

#include <stddef.h>

typedef struct
{
    GPIO_TypeDef    *p_gpio;
    uint16_t        u16_pin;
    bool            b_active_high;
    st_key_config_t st_timing;
} st_key_pin_config_t;

/* 增加表项即可扩展按键，ID 与表索引保持一致。 */
static const st_key_pin_config_t g_a_key_pins[] =
{
    {KEY0_GPIO_Port, KEY0_Pin, false, {0U, 5U, 15U, 1000U}}
};

enum
{
    BSP_ADAPTER_KEY_COUNT =
        sizeof(g_a_key_pins) / sizeof(g_a_key_pins[0])
};

typedef struct
{
    st_key_driver_t           *p_driver;
    st_key_handler_t          *p_handler;
    const st_key_pin_config_t *p_pin;
    pf_key_irq_callback_t      pf_irq_callback;
    void                     *p_irq_object;
} st_key_adapter_port_t;

static st_key_adapter_port_t g_a_key_port[BSP_ADAPTER_KEY_COUNT];

/**
 * @brief 读取已校验的物理按键 GPIO。
 * @param[in] p_context 平台对象指针(包含GPIO端口和引脚等信息)
 * @param[out] p_high 原始高电平标记。
 * @return en_key_status_t 读取结果。
 */
static en_key_status_t bsp_adapter_key_read_level(
    void *p_context,
    bool *p_high)
{
    const st_key_adapter_port_t *p_port = p_context;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_port) || (NULL == p_high) ||
        (NULL == p_port->p_pin) || (NULL == p_port->p_pin->p_gpio) ||
        (0U == p_port->p_pin->u16_pin))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    *p_high = (GPIO_PIN_SET == core_gpio_read_pin_direct(
        p_port->p_pin->p_gpio, p_port->p_pin->u16_pin));
    return BSP_KEY_OK;
}

/**
 * @brief 读取 HAL 的单调毫秒时间。
 * @param[in] p_context 平台对象指针(包含GPIO端口和引脚等信息)
 * @param[out] p_now_ms 当前毫秒值。
 * @return en_key_status_t 读取结果。
 */
static en_key_status_t bsp_adapter_key_get_ms(
    void     *p_context,
    uint32_t *p_now_ms)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_context) || (NULL == p_now_ms))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    *p_now_ms = HAL_GetTick();
    return BSP_KEY_OK;
}

/**
 * @brief 保存原中断状态并进入临界区。
 * @param[out] p_irq_state 调用前的中断屏蔽状态。
 * @return en_key_status_t 临界区状态。
 */
static en_key_status_t bsp_adapter_key_critical_enter(
    uint32_t *p_irq_state)
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

/**
 * @brief 恢复进入临界区前的中断状态。
 * @param[in] u32_irq_state 保存的 PRIMASK 值。
 * @return en_key_status_t 恢复结果。
 */
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

/**
 * @brief 将 Handler 通知函数和对象挂载到 Adapter 实例。
 * @param[in] p_context 平台对象指针(包含GPIO端口和引脚等信息)。
 * @param[in] pf_callback 按键中断通知函数。
 * @param[in] p_object 本键 Driver 对象。
 * @return en_key_status_t 挂载结果。
 */
static en_key_status_t bsp_adapter_key_irq_attach(
    void                 *p_context,
    pf_key_irq_callback_t pf_callback,
    void                 *p_object)
{
    st_key_adapter_port_t *p_port = p_context;
    uint32_t              u32_irq_state;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_port) || (NULL == p_port->p_handler) ||
        (NULL == p_port->p_pin) || (NULL == pf_callback) ||
        (NULL == p_object))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    if (PLATFORM_CRITICAL_OK != platform_critical_enter(&u32_irq_state))
    {
        return BSP_KEY_ERR_IO;
    }
    if (NULL != p_port->pf_irq_callback)
    {
        (void)platform_critical_exit(u32_irq_state);
        return BSP_KEY_ERR_BUSY;
    }
    p_port->p_irq_object    = p_object;
    p_port->pf_irq_callback = pf_callback;
    (void)platform_critical_exit(u32_irq_state);
    return BSP_KEY_OK;
}

/**
 * @brief 解除指定按键的中断绑定。
 * @param[in] p_context 平台对象指针(包含GPIO端口和引脚等信息)。
 * @param[in] p_object 本键 Driver 对象。
 * @return en_key_status_t 解除结果。
 */
static en_key_status_t bsp_adapter_key_irq_detach(
    void *p_context,
    void *p_object)
{
    st_key_adapter_port_t *p_port = p_context;
    uint32_t              u32_irq_state;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_port) || (NULL == p_object))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    if (PLATFORM_CRITICAL_OK != platform_critical_enter(&u32_irq_state))
    {
        return BSP_KEY_ERR_IO;
    }
    if ((NULL == p_port->pf_irq_callback) ||
        (p_object != p_port->p_irq_object))
    {
        (void)platform_critical_exit(u32_irq_state);
        return BSP_KEY_ERR_STATE;
    }
    p_port->pf_irq_callback = NULL;
    p_port->p_irq_object    = NULL;
    (void)platform_critical_exit(u32_irq_state);
    return BSP_KEY_OK;
}

/**
 * @brief 逆序释放全部实例，解除失败的对象保持有效以便重试。
 * @param[in,out] p_dev Wrapper 设备描述，user_data 指向平台对象数组。
 * @return en_key_status_t 首个释放错误。
 */
static en_key_status_t bsp_adapter_key_deinit(st_key_drv_t *p_dev)
{
    st_key_adapter_port_t *p_ports =
        (NULL != p_dev) ? p_dev->p_user_data : NULL;
    uint32_t        u32_index;
    en_key_status_t en_status;
    en_key_status_t en_result = BSP_KEY_OK;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_ports)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    if ((NULL != p_ports[0].p_handler) &&
        p_ports[0].p_handler->b_initialized)
    {
        en_status = bsp_key_handler_deinit(p_ports[0].p_handler);
        if (BSP_KEY_OK != en_status)
        {
            return en_status;
        }
    }
    for (u32_index = BSP_ADAPTER_KEY_COUNT; 0U < u32_index; u32_index--)
    {
        if (NULL != p_ports[u32_index - 1U].p_driver)
        {
            (void)bsp_key_driver_deinit(p_ports[u32_index - 1U].p_driver);
        }
    }
    return en_result;
}

/* 平台接口和运行接口在初始化前完成组装。 */
static const st_key_port_ops_t g_st_key_port_ops =
{
    bsp_adapter_key_read_level
};

static const st_key_runtime_ops_t g_st_key_runtime_ops =
{
    bsp_adapter_key_get_ms,
    bsp_adapter_key_critical_enter,
    bsp_adapter_key_critical_exit,
    bsp_adapter_key_irq_attach,
    bsp_adapter_key_irq_detach
};

/**
 * @brief 绑定各键 Driver，再挂载共用 Handler，失败时清理。
 * @param[in,out] p_dev Wrapper 设备描述，user_data 指向平台对象数组。
 * @return en_key_status_t 初始化或清理错误。
 */
static en_key_status_t bsp_adapter_key_bind(st_key_drv_t *p_dev)
{
    st_key_adapter_port_t *p_ports =
        (NULL != p_dev) ? p_dev->p_user_data : NULL;
    uint32_t        u32_index;
    en_key_status_t en_status = BSP_KEY_OK;
    en_key_status_t en_cleanup;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_ports)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    for (u32_index = 0U; u32_index < BSP_ADAPTER_KEY_COUNT; u32_index++)
    {
        uint32_t u32_previous;
        const st_key_pin_config_t *p_pin = &g_a_key_pins[u32_index];
        if ((NULL == p_pin->p_gpio) || (0U == p_pin->u16_pin) ||
            (0U != (p_pin->u16_pin & (p_pin->u16_pin - 1U))) ||
            (u32_index != p_pin->st_timing.u32_key_id))
        {
            return BSP_KEY_ERR_ARGUMENT;
        }
        for (u32_previous = 0U; u32_previous < u32_index; u32_previous++)
        {
            if (p_pin->u16_pin == g_a_key_pins[u32_previous].u16_pin)
            {
                return BSP_KEY_ERR_ARGUMENT;
            }
        }
        en_status = bsp_key_handler_get_objects(u32_index,
            &p_ports[u32_index].p_driver,
            &p_ports[u32_index].p_handler);
        if (BSP_KEY_OK != en_status)
        {
            return en_status;
        }
        if (p_ports[u32_index].p_driver->b_initialized ||
            p_ports[u32_index].p_handler->b_initialized)
        {
            return BSP_KEY_ERR_STATE;
        }
    }
    //--------------------------参数检查 end--------------------------//

    for (u32_index = 0U; u32_index < BSP_ADAPTER_KEY_COUNT; u32_index++)
    {
        st_key_adapter_port_t *p_port = &p_ports[u32_index];
        p_port->p_pin     = &g_a_key_pins[u32_index];

        en_status = bsp_key_driver_init(
            p_ports[u32_index].p_driver,
            &g_st_key_port_ops,
            p_port,
            p_port->p_pin->b_active_high);
        if (BSP_KEY_OK != en_status)
        {
            break;
        }
        p_port->p_driver->st_config = p_port->p_pin->st_timing;
    }
    if (BSP_KEY_OK == en_status)
    {
        en_status = bsp_key_handler_init(
            p_ports[0].p_handler,
            p_ports[0].p_driver,
            BSP_ADAPTER_KEY_COUNT,
            &g_st_key_runtime_ops,
            &p_ports[0]);
    }
    if (BSP_KEY_OK != en_status)
    {
        en_cleanup = bsp_adapter_key_deinit(p_dev);
        if (BSP_KEY_OK != en_cleanup)
        {
            return en_cleanup;
        }
    }
    return en_status;
}

/**
 * @brief 扫描各按键，一个按键失败不阻止其他按键扫描。
 * @param[in,out] p_dev Wrapper 设备描述，user_data 指向平台对象数组。
 * @return en_key_status_t 首个扫描错误。
 */
static en_key_status_t bsp_adapter_key_scan(st_key_drv_t *p_dev)
{
    st_key_adapter_port_t *p_ports =
        (NULL != p_dev) ? p_dev->p_user_data : NULL;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_ports)
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return bsp_key_handler_scan(p_ports[0].p_handler);
}

/**
 * @brief 从指定 Handler 取出并清除事件位。
 * @param[in] p_dev Wrapper 设备描述，user_data 指向平台对象数组。
 * @param[in] u32_key_id 按键 ID。
 * @param[out] p_flags 动作位集合。
 * @return en_key_status_t 查询结果。
 */
static en_key_status_t bsp_adapter_key_take_events(
    st_key_drv_t *p_dev,
    uint32_t      u32_key_id,
    uint32_t     *p_flags)
{
    st_key_adapter_port_t *p_ports =
        (NULL != p_dev) ? p_dev->p_user_data : NULL;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_ports) || (NULL == p_flags) ||
        (BSP_ADAPTER_KEY_COUNT <= u32_key_id))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    if ((NULL == p_ports[u32_key_id].p_handler) ||
        (!p_ports[u32_key_id].p_handler->b_initialized))
    {
        return BSP_KEY_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    return bsp_key_handler_take_events(
        p_ports[0].p_handler, u32_key_id, p_flags);
}

/**
 * @brief 在 ISR 中按 EXTI 引脚调用已挂载的对象。
 * @param[in] p_dev Wrapper 设备描述，user_data 指向平台对象数组。
 * @param[in] u16_pin 单一 EXTI 引脚掩码。
 * @return en_key_status_t 中断通知结果。
 */
static en_key_status_t bsp_adapter_key_irq_notify(
    st_key_drv_t *p_dev,
    uint16_t      u16_pin)
{
    st_key_adapter_port_t *p_ports =
        (NULL != p_dev) ? p_dev->p_user_data : NULL;
    uint32_t u32_index;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_ports) || (0U == u16_pin) ||
        (0U != (u16_pin & (u16_pin - 1U))))
    {
        return BSP_KEY_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    for (u32_index = 0U; u32_index < BSP_ADAPTER_KEY_COUNT; u32_index++)
    {
        st_key_adapter_port_t *p_port = &p_ports[u32_index];
        if ((u16_pin == g_a_key_pins[u32_index].u16_pin) &&
            (NULL != p_port->pf_irq_callback))
        {
            return p_port->pf_irq_callback(p_port->p_irq_object);
        }
    }
    return BSP_KEY_ERR_STATE;
}

/**
 * @brief 组装设备描述并复制注册到 Wrapper 的 0 号槽位。
 * @param[in] 无；平台接口、输入对象和设备操作由 Adapter 持有。
 * @return en_key_status_t 接口绑定及注册结果，成功后可直接扫描。
 */
en_key_status_t drv_adapter_key_register(void)
{
    st_key_drv_t st_key_drv =
    {
        .u32_idx                = 0U,
        .u32_dev_id             = 0U,
        .p_user_data            = g_a_key_port,
        .pf_key_drv_scan        = bsp_adapter_key_scan,
        .pf_key_drv_deinit      = bsp_adapter_key_deinit,
        .pf_key_drv_take_events = bsp_adapter_key_take_events,
        .pf_key_drv_irq_notify  = bsp_adapter_key_irq_notify
    };

    en_key_status_t en_status;
    en_key_status_t en_cleanup;

    //-------------------------参数检查 begin-------------------------//
    /* 无入参；绑定函数检查配置及对象状态，拒绝重复挂载。 */
    //--------------------------参数检查 end--------------------------//

    en_status = bsp_adapter_key_bind(&st_key_drv);
    if (BSP_KEY_OK != en_status)
    {
        return en_status;
    }
    en_status = bsp_wrapper_key_reg(0U, &st_key_drv);
    if (BSP_KEY_OK != en_status)
    {
        en_cleanup = bsp_adapter_key_deinit(&st_key_drv);
        if (BSP_KEY_OK != en_cleanup)
        {
            return en_cleanup;
        }
    }
    return en_status;
}
