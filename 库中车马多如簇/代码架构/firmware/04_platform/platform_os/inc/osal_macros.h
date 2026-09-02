#ifndef OSAL_MACROS_H
#define OSAL_MACROS_H

#define OSAL_RETURN_IF_NULL(pointer)             \
    do                                           \
    {                                            \
        if ((pointer) == NULL)                   \
        {                                        \
            return OSAL_ERR_INVALID_POINTER;     \
        }                                        \
    } while (0)

#define OSAL_RETURN_IF_FALSE(condition, error)   \
    do                                           \
    {                                            \
        if (!(condition))                        \
        {                                        \
            return (error);                      \
        }                                        \
    } while (0)

#endif
