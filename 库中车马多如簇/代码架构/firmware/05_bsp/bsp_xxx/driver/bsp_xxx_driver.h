#ifndef BSP_XXX_DRIVER_H
#define BSP_XXX_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BSP_XXX_OK = 0,
    BSP_XXX_ERROR = -1,
    BSP_XXX_ERR_ARGUMENT = -2,
    BSP_XXX_ERR_STATE = -3
} bsp_xxx_status_t;

typedef struct
{
    bsp_xxx_status_t (*init)(void *context);
    bsp_xxx_status_t (*control)(void *context, uint32_t command,
                                void *argument);
} bsp_xxx_port_ops_t;

typedef struct
{
    const bsp_xxx_port_ops_t *port_ops;
    void *port_context;
    bool initialized;
} bsp_xxx_driver_t;

bsp_xxx_status_t bsp_xxx_driver_bind(bsp_xxx_driver_t *driver,
                                     const bsp_xxx_port_ops_t *port_ops,
                                     void *port_context);
bsp_xxx_status_t bsp_xxx_driver_init(bsp_xxx_driver_t *driver);
bsp_xxx_status_t bsp_xxx_driver_control(bsp_xxx_driver_t *driver,
                                        uint32_t command, void *argument);

#endif
