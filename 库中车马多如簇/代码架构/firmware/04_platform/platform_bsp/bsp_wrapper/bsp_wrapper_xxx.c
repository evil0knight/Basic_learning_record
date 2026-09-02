#include "bsp_wrapper_xxx.h"

#include <stddef.h>

static bsp_wrapper_xxx_ops_t g_bsp_wrapper_xxx_ops;
static bool g_bsp_wrapper_xxx_registered;

bool bsp_wrapper_xxx_register(const bsp_wrapper_xxx_ops_t *ops)
{
    if ((ops == NULL) || (ops->init == NULL) || (ops->control == NULL) ||
        g_bsp_wrapper_xxx_registered)
    {
        return false;
    }

    g_bsp_wrapper_xxx_ops = *ops;
    g_bsp_wrapper_xxx_registered = true;
    return true;
}

bsp_wrapper_xxx_status_t bsp_wrapper_xxx_init(void)
{
    if (!g_bsp_wrapper_xxx_registered)
    {
        return BSP_WRAPPER_XXX_ERR_STATE;
    }
    return g_bsp_wrapper_xxx_ops.init(g_bsp_wrapper_xxx_ops.context);
}

bsp_wrapper_xxx_status_t bsp_wrapper_xxx_control(uint32_t index,
                                                 uint32_t command,
                                                 void *argument)
{
    if (!g_bsp_wrapper_xxx_registered)
    {
        return BSP_WRAPPER_XXX_ERR_STATE;
    }
    return g_bsp_wrapper_xxx_ops.control(g_bsp_wrapper_xxx_ops.context,
                                         index, command, argument);
}
