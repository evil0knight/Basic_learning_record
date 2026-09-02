#ifndef BSP_XXX_HANDLER_H
#define BSP_XXX_HANDLER_H

#include <stddef.h>
#include <stdint.h>

#include "bsp_xxx_driver.h"

#ifndef BSP_XXX_MAX_INSTANCES
#define BSP_XXX_MAX_INSTANCES 8U
#endif

typedef struct
{
    void (*enter)(void *context);
    void (*exit)(void *context);
    void *context;
} bsp_xxx_lock_t;

typedef struct
{
    bsp_xxx_driver_t *drivers[BSP_XXX_MAX_INSTANCES];
    size_t count;
    const bsp_xxx_lock_t *lock;
} bsp_xxx_handler_t;

bsp_xxx_status_t bsp_xxx_handler_init(bsp_xxx_handler_t *handler,
                                      const bsp_xxx_lock_t *lock);
bsp_xxx_status_t bsp_xxx_handler_register(bsp_xxx_handler_t *handler,
                                          bsp_xxx_driver_t *driver,
                                          uint32_t *index);
bsp_xxx_status_t bsp_xxx_handler_control(bsp_xxx_handler_t *handler,
                                         uint32_t index, uint32_t command,
                                         void *argument);

#endif
