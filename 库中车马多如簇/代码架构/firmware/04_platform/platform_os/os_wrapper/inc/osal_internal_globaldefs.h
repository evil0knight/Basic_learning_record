#ifndef OSAL_INTERNAL_GLOBALDEFS_H
#define OSAL_INTERNAL_GLOBALDEFS_H

#include <string.h>

#include "osal_config.h"
#include "osal_error.h"
#include "osal_macros.h"
#include "osal_types.h"

static inline bool osal_name_is_valid(const char *name)
{
    return (name != NULL) && (memchr(name, '\0', OSAL_NAME_MAX) != NULL);
}

#endif
