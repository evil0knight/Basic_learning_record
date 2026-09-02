#include "bsp_xxx_handler.h"

#include <string.h>

static void bsp_xxx_lock_enter(const bsp_xxx_handler_t *handler)
{
    if ((handler->lock != NULL) && (handler->lock->enter != NULL))
    {
        handler->lock->enter(handler->lock->context);
    }
}

static void bsp_xxx_lock_exit(const bsp_xxx_handler_t *handler)
{
    if ((handler->lock != NULL) && (handler->lock->exit != NULL))
    {
        handler->lock->exit(handler->lock->context);
    }
}

bsp_xxx_status_t bsp_xxx_handler_init(bsp_xxx_handler_t *handler,
                                      const bsp_xxx_lock_t *lock)
{
    if (handler == NULL)
    {
        return BSP_XXX_ERR_ARGUMENT;
    }

    memset(handler->drivers, 0, sizeof(handler->drivers));
    handler->count = 0U;
    handler->lock = lock;
    return BSP_XXX_OK;
}

bsp_xxx_status_t bsp_xxx_handler_register(bsp_xxx_handler_t *handler,
                                          bsp_xxx_driver_t *driver,
                                          uint32_t *index)
{
    size_t position;

    if ((handler == NULL) || (driver == NULL) || (index == NULL) ||
        !driver->initialized)
    {
        return BSP_XXX_ERR_ARGUMENT;
    }

    bsp_xxx_lock_enter(handler);
    for (position = 0U; position < BSP_XXX_MAX_INSTANCES; ++position)
    {
        if (handler->drivers[position] == driver)
        {
            bsp_xxx_lock_exit(handler);
            return BSP_XXX_ERR_STATE;
        }
        if (handler->drivers[position] == NULL)
        {
            handler->drivers[position] = driver;
            handler->count++;
            *index = (uint32_t)position;
            bsp_xxx_lock_exit(handler);
            return BSP_XXX_OK;
        }
    }
    bsp_xxx_lock_exit(handler);
    return BSP_XXX_ERR_STATE;
}

bsp_xxx_status_t bsp_xxx_handler_control(bsp_xxx_handler_t *handler,
                                         uint32_t index, uint32_t command,
                                         void *argument)
{
    if ((handler == NULL) || (index >= BSP_XXX_MAX_INSTANCES) ||
        (handler->drivers[index] == NULL))
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    return bsp_xxx_driver_control(handler->drivers[index], command, argument);
}
