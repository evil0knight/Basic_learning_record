#include "bsp_xxx_driver.h"

#include <stddef.h>

bsp_xxx_status_t bsp_xxx_driver_bind(bsp_xxx_driver_t *driver,
                                     const bsp_xxx_port_ops_t *port_ops,
                                     void *port_context)
{
    if ((driver == NULL) || (port_ops == NULL) ||
        (port_ops->init == NULL) || (port_ops->control == NULL))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    if (driver->initialized)
    {
        return BSP_XXX_ERR_STATE;
    }

    driver->port_ops = port_ops;
    driver->port_context = port_context;
    return BSP_XXX_OK;
}

bsp_xxx_status_t bsp_xxx_driver_init(bsp_xxx_driver_t *driver)
{
    bsp_xxx_status_t status;

    if ((driver == NULL) || (driver->port_ops == NULL))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    if (driver->initialized)
    {
        return BSP_XXX_OK;
    }

    status = driver->port_ops->init(driver->port_context);
    if (status == BSP_XXX_OK)
    {
        driver->initialized = true;
    }
    return status;
}

bsp_xxx_status_t bsp_xxx_driver_control(bsp_xxx_driver_t *driver,
                                        uint32_t command, void *argument)
{
    if ((driver == NULL) || !driver->initialized ||
        (driver->port_ops == NULL))
    {
        return BSP_XXX_ERR_STATE;
    }
    return driver->port_ops->control(driver->port_context, command, argument);
}
