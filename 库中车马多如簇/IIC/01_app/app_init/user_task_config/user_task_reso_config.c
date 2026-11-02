/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file user_task_reso_config.c
 *
 * @par dependencies
 * - user_task_reso_config.h
 * - i2c_port_os.h
 *
 * @author YA
 *
 * @brief I2C 通道独占的 OSAL 资源配置实现。
 *
 * Processing flow:
 *
 * 配置表 -> 资源创建 -> 注册到 I2C Port；失败时释放已创建资源。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
//****************************** Includes begin ******************************//
#include "user_task_reso_config.h"

#include "i2c_port_os.h"
//******************************* Includes end *******************************//

//************************** Static Variables begin **************************//
user_mutex_config_t g_user_mutexes[USER_MUTEX_IDX_MAX] =
{
    [USER_MUTEX_IDX_I2C1] = {true, NULL}
};

user_sema_config_t g_user_semaphores[USER_SEMA_IDX_MAX] =
{
    [USER_SEMA_IDX_I2C1_DONE] = {true, true, 1U, 0U, NULL}
};
//*************************** Static Variables end ***************************//

//************************ Function Definitions begin ************************//

//****************************** 公共接口 begin ******************************//
/**
 * @brief 释放 I2C 总线锁与 DMA 完成信号量。
 *
 * @param[in] None
 * @return void : 无返回值。
 */
void user_os_resources_deinit(void)
{
    size_t index;                          /* 遍历资源配置表的索引。 */

    /* 释放信号量并清空句柄，避免重复释放。 */
    for (index = 0U; index < USER_SEMA_IDX_MAX; ++index)
    {
        if (NULL != g_user_semaphores[index].handle)
        {
            (void)osal_sema_delete(g_user_semaphores[index].handle);
            g_user_semaphores[index].handle = NULL;
        }
    }

    /* 释放互斥量并清空句柄，避免重复释放。 */
    for (index = 0U; index < USER_MUTEX_IDX_MAX; ++index)
    {
        if (NULL != g_user_mutexes[index].handle)
        {
            (void)osal_mutex_delete(g_user_mutexes[index].handle);
            g_user_mutexes[index].handle = NULL;
        }
    }
}

/**
 * @brief 创建资源表中启用的互斥量与信号量。
 *
 * Steps:
 *  1. 创建启用的互斥量。
 *  2. 创建启用的信号量。
 *
 * @param[in] None
 * @return osal_status_t : 创建状态。
 */
osal_status_t user_os_resources_init(void)
{
    osal_status_t status;                  /* 保存当前结果。 */
    size_t index;                          /* 遍历资源配置表的索引。 */

    /* 创建配置表的互斥量。 */
    for (index = 0U; index < USER_MUTEX_IDX_MAX; ++index)
    {
        /* 未启用的互斥量不申请内核资源。 */
        if (false == g_user_mutexes[index].enabled)
        {
            continue;
        }

        /* 创建互斥量并保存句柄。 */
        status = osal_mutex_create(&g_user_mutexes[index].handle);
        if (OSAL_SUCCESS != status)
        {
            /* 创建失败时释放已创建的全部资源。 */
            user_os_resources_deinit();
            return status;
        }
    }

    /* 创建配置表的二值信号量。 */
    for (index = 0U; index < USER_SEMA_IDX_MAX; ++index)
    {
        /* 未启用的信号量不申请内核资源。 */
        if (false == g_user_semaphores[index].enabled)
        {
            continue;
        }

        /* 创建信号量并保存句柄。 */
        status = osal_sema_binary_create(&g_user_semaphores[index].handle);
        if (OSAL_SUCCESS != status)
        {
            /* 创建失败时释放已创建的全部资源。 */
            user_os_resources_deinit();
            return status;
        }
    }

    /* 所有启用的 OSAL 资源均创建成功。 */
    return OSAL_SUCCESS;
}

//******************************* 公共接口 end *******************************//

//****************************** 辅助接口 begin ******************************//
/**
 * @brief 获取指定互斥量句柄。
 *
 * @param[in] index : 互斥量索引。
 * @return osal_mutex_handle_t : 互斥量句柄，无效索引返回 NULL。
 */
osal_mutex_handle_t user_mutex_get(user_mutex_idx_t index)
{
    return (index < USER_MUTEX_IDX_MAX) ? g_user_mutexes[index].handle : NULL;
}

/**
 * @brief 获取指定信号量句柄。
 *
 * @param[in] index : 信号量索引。
 * @return osal_sema_handle_t : 信号量句柄，无效索引返回 NULL。
 */
osal_sema_handle_t user_sema_get(user_sema_idx_t index)
{
    return (index < USER_SEMA_IDX_MAX)
               ? g_user_semaphores[index].handle
               : NULL;
}

/**
 * @brief 把资源表里的 I2C 锁与完成信号量注册给 I2C Port。
 *
 * Steps:
 *  1. 从资源表取出 I2C1 的互斥量与完成信号量。
 *  2. 注册到 I2C Port，之后 Port 内部自己加锁和等完成。
 *
 * @param[in] None
 * @return void : 无返回值。
 */
void user_i2c_resource_register(void)
{
    (void)core_i2c_register_mutex(CORE_I2C_PORT_ENCODER1,
                                  user_mutex_get(USER_MUTEX_IDX_I2C1));
    (void)core_i2c_register_semaphore(CORE_I2C_PORT_ENCODER1,
                                      user_sema_get(USER_SEMA_IDX_I2C1_DONE));
}
//******************************* 辅助接口 end *******************************//
