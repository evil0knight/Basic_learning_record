#ifndef OS_FREERTOS_H
#define OS_FREERTOS_H

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "osal_config.h"
#include "osal_types.h"

#if (OSAL_RTOS != OSAL_FREERTOS)
#error "This implementation requires OSAL_RTOS == OSAL_FREERTOS"
#endif

#define OSAL_PORT_IS_IN_ISR() (xPortIsInsideInterrupt() != pdFALSE)

static inline TickType_t osal_port_ticks(osal_tick_t ticks)
{
    return (ticks == OSAL_WAIT_FOREVER) ? portMAX_DELAY : (TickType_t)ticks;
}

#endif
