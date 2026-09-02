#ifndef BSP_WRAPPER_XXX_H
#define BSP_WRAPPER_XXX_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BSP_WRAPPER_XXX_OK = 0,
    BSP_WRAPPER_XXX_ERROR = -1,
    BSP_WRAPPER_XXX_ERR_ARGUMENT = -2,
    BSP_WRAPPER_XXX_ERR_STATE = -3
} bsp_wrapper_xxx_status_t;

typedef struct
{
    bsp_wrapper_xxx_status_t (*init)(void *context);
    bsp_wrapper_xxx_status_t (*control)(void *context, uint32_t index,
                                        uint32_t command, void *argument);
    void *context;
} bsp_wrapper_xxx_ops_t;

bool bsp_wrapper_xxx_register(const bsp_wrapper_xxx_ops_t *ops);
bsp_wrapper_xxx_status_t bsp_wrapper_xxx_init(void);
bsp_wrapper_xxx_status_t bsp_wrapper_xxx_control(uint32_t index,
                                                 uint32_t command,
                                                 void *argument);

#endif
