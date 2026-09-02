#ifndef OSAL_CONFIG_H
#define OSAL_CONFIG_H

#define OSAL_FREERTOS 1

#ifndef OSAL_RTOS
#define OSAL_RTOS OSAL_FREERTOS
#endif

#ifndef OSAL_NAME_MAX
#define OSAL_NAME_MAX 16U
#endif

#endif
