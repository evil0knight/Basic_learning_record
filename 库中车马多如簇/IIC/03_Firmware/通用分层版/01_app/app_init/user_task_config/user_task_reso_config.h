/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_task_reso_config.h
 *
 * @par dependencies
 * - osal.h
 * - stdbool.h
 * - stddef.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief I2C 通道独占的 OSAL 资源（总线锁、DMA 完成信号量）配置接口。
 *
 * Processing flow:
 *
 * 配置表 -> user_os_resources_init -> user_i2c_resource_register。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef USER_TASK_RESO_CONFIG_H
#define USER_TASK_RESO_CONFIG_H

//****************************** Includes begin ******************************//
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "osal.h"
//******************************* Includes end *******************************//

//****************************** Typedefs begin ******************************//
typedef enum
{
    USER_MUTEX_IDX_I2C1 = 0,
    USER_MUTEX_IDX_MAX
} user_mutex_idx_t;

typedef enum
{
    USER_SEMA_IDX_I2C1_DONE = 0,
    USER_SEMA_IDX_MAX
} user_sema_idx_t;

typedef struct
{
    bool enabled;
    osal_mutex_handle_t handle;
} user_mutex_config_t;

typedef struct
{
    bool enabled;
    bool binary;
    uint32_t maximum;
    uint32_t initial;
    osal_sema_handle_t handle;
} user_sema_config_t;
//******************************* Typedefs end *******************************//

//***************************** Declaring begin ******************************//
extern user_mutex_config_t g_user_mutexes[USER_MUTEX_IDX_MAX];
extern user_sema_config_t g_user_semaphores[USER_SEMA_IDX_MAX];

/**
 * @brief 创建资源表中启用的互斥量与信号量。
 *
 */
osal_status_t user_os_resources_init(void);
/**
 * @brief 释放已创建的互斥量与信号量。
 *
 */
void user_os_resources_deinit(void);
/**
 * @brief 获取指定互斥量句柄。
 *
 */
osal_mutex_handle_t user_mutex_get(user_mutex_idx_t index);
/**
 * @brief 获取指定信号量句柄。
 *
 */
osal_sema_handle_t user_sema_get(user_sema_idx_t index);
/**
 * @brief 把资源表里的 I2C 锁与完成信号量注册给 I2C Port。
 *
 */
void user_i2c_resource_register(void);
//****************************** Declaring end *******************************//

#endif /* USER_TASK_RESO_CONFIG_H */
