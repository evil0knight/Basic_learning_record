#ifndef OSAL_ERROR_H
#define OSAL_ERROR_H

#include <stdint.h>

typedef enum
{
    OSAL_SUCCESS = 0,
    OSAL_ERROR = -1,
    OSAL_ERR_INVALID_POINTER = -2,
    OSAL_ERR_INVALID_SIZE = -3,
    OSAL_ERR_INVALID_ARGUMENT = -4,
    OSAL_ERR_NAME_TOO_LONG = -5,
    OSAL_ERR_TIMEOUT = -6,
    OSAL_ERR_QUEUE_FULL = -7,
    OSAL_ERR_QUEUE_EMPTY = -8,
    OSAL_ERR_IN_ISR = -9,
    OSAL_ERR_NO_MEMORY = -10
} osal_status_t;

#endif
