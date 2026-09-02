#include "bsp_adapter_port_xxx.h"

#include "bsp_wrapper_xxx.h"
#include "bsp_xxx_driver.h"
#include "bsp_xxx_handler.h"

#include <stddef.h>

typedef struct
{
    bsp_xxx_driver_t driver;
    bsp_xxx_handler_t handler;
    uint32_t index;
} bsp_adapter_xxx_context_t;

static bsp_adapter_xxx_context_t g_bsp_adapter_xxx;

static bsp_xxx_status_t bsp_adapter_xxx_port_init(void *context)
{
    (void)context;
    /* 在这里调用具体 MCU/HAL 的初始化接口。 */
    return BSP_XXX_ERROR;
}

static bsp_xxx_status_t bsp_adapter_xxx_port_control(void *context,
                                                     uint32_t command,
                                                     void *argument)
{
    (void)context;
    (void)command;
    (void)argument;
    /* 在这里把通用命令转换为具体 MCU/HAL 操作。 */
    return BSP_XXX_ERROR;
}

static const bsp_xxx_port_ops_t g_bsp_adapter_xxx_port_ops =
{
    bsp_adapter_xxx_port_init,
    bsp_adapter_xxx_port_control
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

static bsp_wrapper_xxx_status_t bsp_adapter_xxx_init(void *context)
{
    bsp_adapter_xxx_context_t *adapter = context;
    bsp_xxx_status_t status;

    if (adapter == NULL)
    {
        return BSP_WRAPPER_XXX_ERR_ARGUMENT;
    }

    status = bsp_xxx_driver_bind(&adapter->driver,
                                 &g_bsp_adapter_xxx_port_ops, NULL);
    if (status != BSP_XXX_OK)
    {
        return bsp_adapter_xxx_status_convert(status);
    }

    status = bsp_xxx_driver_init(&adapter->driver);
    if (status != BSP_XXX_OK)
    {
        return bsp_adapter_xxx_status_convert(status);
    }

    status = bsp_xxx_handler_init(&adapter->handler, NULL);
    if (status != BSP_XXX_OK)
    {
        return bsp_adapter_xxx_status_convert(status);
    }

    status = bsp_xxx_handler_register(&adapter->handler, &adapter->driver,
                                      &adapter->index);
    return bsp_adapter_xxx_status_convert(status);
}

static bsp_wrapper_xxx_status_t bsp_adapter_xxx_control(
    void *context, uint32_t index, uint32_t command, void *argument)
{
    bsp_adapter_xxx_context_t *adapter = context;

    if (adapter == NULL)
    {
        return BSP_WRAPPER_XXX_ERR_ARGUMENT;
    }

    return bsp_adapter_xxx_status_convert(
        bsp_xxx_handler_control(&adapter->handler, index, command, argument));
}

void bsp_adapter_xxx_register(void)
{
    static const bsp_wrapper_xxx_ops_t ops =
    {
        bsp_adapter_xxx_init,
        bsp_adapter_xxx_control,
        &g_bsp_adapter_xxx
    };

    (void)bsp_wrapper_xxx_register(&ops);
}
