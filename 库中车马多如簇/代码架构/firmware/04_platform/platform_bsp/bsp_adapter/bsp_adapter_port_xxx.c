/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_adapter_port_xxx.c
 *
 * @par dependencies
 * - bsp_adapter_port_xxx.h
 * - bsp_wrapper_xxx.h
 * - bsp_xxx_driver.h
 * - bsp_xxx_handler.h
 * - stddef.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 组装 XXX 的平台接口，并将 Handler 挂载到 Wrapper。
 *
 * Processing flow:
 *
 * 平台接口实现 -> Driver bind -> Handler init -> Wrapper register。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#include "bsp_adapter_port_xxx.h"

#include "../bsp_wrapper/bsp_wrapper_xxx.h"
#include "../../../05_bsp/bsp_xxx/driver/bsp_xxx_driver.h"
#include "../../../05_bsp/bsp_xxx/handler/bsp_xxx_handler.h"

#include <stddef.h>

typedef struct
{
    bsp_xxx_driver_t driver;
    bsp_xxx_handler_t handler;
    uint32_t index;
} bsp_adapter_xxx_context_t;

static bsp_adapter_xxx_context_t g_bsp_adapter_xxx;

/**
 * @brief 初始化 XXX 底层接口。
 * @param[in] p_context 平台上下文指针。
 * @return bsp_xxx_status_t 操作状态。
 */
static bsp_xxx_status_t bsp_adapter_xxx_port_init(void *p_context)
{
    if (p_context == NULL)
    {
        return BSP_XXX_ERR_ARGUMENT;
    }

    return BSP_XXX_OK;
}

/**
 * @brief 执行 XXX 底层控制操作。
 * @param[in] p_context 平台上下文指针。
 * @param[in] command 控制命令。
 * @param[in] p_argument 命令参数指针。
 * @return bsp_xxx_status_t 操作状态。
 */
static bsp_xxx_status_t bsp_adapter_xxx_port_control(
    void *p_context,
    uint32_t command,
    void *p_argument)
{
    if (p_context == NULL)
    {
        return BSP_XXX_ERR_ARGUMENT;
    }

    (void)command;
    (void)p_argument;
    return BSP_XXX_OK;
}

/**
 * @brief 进入 XXX 底层临界区。
 * @return bsp_xxx_status_t 操作状态。
 */
static bsp_xxx_status_t bsp_adapter_xxx_critical_enter(void)
{
    return BSP_XXX_OK;
}

/**
 * @brief 退出 XXX 底层临界区。
 * @return bsp_xxx_status_t 操作状态。
 */
static bsp_xxx_status_t bsp_adapter_xxx_critical_exit(void)
{
    return BSP_XXX_OK;
}

static const bsp_xxx_port_ops_t g_bsp_adapter_xxx_port_ops =
{
    .pf_init = bsp_adapter_xxx_port_init,
    .pf_control = bsp_adapter_xxx_port_control,
    .pf_critical_enter = bsp_adapter_xxx_critical_enter,
    .pf_critical_exit = bsp_adapter_xxx_critical_exit
};

static bsp_wrapper_xxx_status_t bsp_adapter_xxx_status_convert(
    bsp_xxx_status_t status)
{
    switch (status)
    {
        case BSP_XXX_OK:
            return BSP_WRAPPER_XXX_OK;
        case BSP_XXX_ERR_ARGUMENT:
            return BSP_WRAPPER_XXX_ERR_ARGUMENT;
        case BSP_XXX_ERR_STATE:
            return BSP_WRAPPER_XXX_ERR_STATE;
        default:
            return BSP_WRAPPER_XXX_ERROR;
    }
}

/**
 * @brief 初始化 XXX Wrapper 对应的 Handler。
 * @param[in] p_context Adapter 上下文指针。
 * @return bsp_wrapper_xxx_status_t 操作状态。
 */
static bsp_wrapper_xxx_status_t bsp_adapter_xxx_init(void *p_context)
{
    bsp_adapter_xxx_context_t *p_adapter;
    bsp_xxx_status_t status;

    if (p_context == NULL)
    {
        return BSP_WRAPPER_XXX_ERR_ARGUMENT;
    }

    p_adapter = (bsp_adapter_xxx_context_t *)p_context;
    status = bsp_xxx_handler_init(&p_adapter->handler,
                                  &p_adapter->driver,
                                  &g_bsp_adapter_xxx_port_ops,
                                  p_adapter);
    if (status != BSP_XXX_OK)
    {
        return bsp_adapter_xxx_status_convert(status);
    }

    p_adapter->index = 0U;
    return BSP_WRAPPER_XXX_OK;
}

/**
 * @brief 转发 XXX Wrapper 控制命令。
 * @param[in] p_context Adapter 上下文指针。
 * @param[in] index Driver 索引。
 * @param[in] command 控制命令。
 * @param[in] p_argument 命令参数指针。
 * @return bsp_wrapper_xxx_status_t 操作状态。
 */
static bsp_wrapper_xxx_status_t bsp_adapter_xxx_control(
    void *p_context,
    uint32_t index,
    uint32_t command,
    void *p_argument)
{
    bsp_adapter_xxx_context_t *p_adapter;

    if (p_context == NULL)
    {
        return BSP_WRAPPER_XXX_ERR_ARGUMENT;
    }

    p_adapter = (bsp_adapter_xxx_context_t *)p_context;
    return bsp_adapter_xxx_status_convert(
        bsp_xxx_handler_control(&p_adapter->handler,
                                index,
                                command,
                                p_argument));
}

/**
 * @brief 将 XXX Adapter 注册到 Wrapper。
 * @return void。
 */
void bsp_adapter_xxx_register(void)
{
    static const bsp_wrapper_xxx_ops_t ops =
    {
        .init = bsp_adapter_xxx_init,
        .control = bsp_adapter_xxx_control,
        .context = &g_bsp_adapter_xxx
    };

    (void)bsp_wrapper_xxx_register(&ops);
}
