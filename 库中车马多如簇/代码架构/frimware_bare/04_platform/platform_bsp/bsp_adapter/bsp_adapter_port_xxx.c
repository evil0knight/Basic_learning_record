/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_adapter_port_xxx.c
 *
 * @par dependencies
 * - bsp_adapter_port_xxx.h
 * - bsp_wrapper_xxx.h
 * - bsp_xxx_driver.h
 * - stddef.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 组装 XXX 的平台 Port 接口表，并把 Handler 挂载到 Wrapper。
 *
 * Processing flow:
 *
 * 平台接口实现 -> Driver inst -> Handler inst -> Wrapper register。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "bsp_adapter_port_xxx.h"

#include "../bsp_wrapper/bsp_wrapper_xxx.h"
#include "../../../05_bsp/bsp_xxx/driver/bsp_xxx_driver.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//
static bsp_xxx_status_t bsp_adapter_xxx_port_init(void *p_context);
static bsp_xxx_status_t bsp_adapter_xxx_port_control(
    void *p_context,
    uint32_t command,
    void *p_argument);
static bsp_xxx_status_t bsp_adapter_xxx_critical_enter(void);
static bsp_xxx_status_t bsp_adapter_xxx_critical_exit(void);
static bsp_wrapper_xxx_status_t bsp_adapter_xxx_status_convert(
    bsp_xxx_status_t status);
static bsp_wrapper_xxx_status_t bsp_adapter_xxx_init(void *p_context);
static bsp_wrapper_xxx_status_t bsp_adapter_xxx_control(
    void *p_context,
    uint32_t index,
    uint32_t command,
    void *p_argument);
//************************* Function Prototypes end **************************//

//************************** Static Variables begin **************************//
/* Driver 实例，单设备模板只维护一份；无 Handler 时驱动对象就放在这里。 */
static bsp_xxx_driver_t g_bsp_adapter_xxx_driver;

/* 平台 Port 接口表，交给 Driver 通过 pf_* 调用。 */
static const bsp_xxx_port_ops_t g_bsp_adapter_xxx_port_ops =
{
    .pf_init = bsp_adapter_xxx_port_init,
    .pf_control = bsp_adapter_xxx_port_control,
    .pf_critical_enter = bsp_adapter_xxx_critical_enter,
    .pf_critical_exit = bsp_adapter_xxx_critical_exit
};
//*************************** Static Variables end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 将 XXX Adapter 注册到 BSP Wrapper。
 *
 * Steps:
 *  1. 组装 Wrapper 需要的 init、control 和 context 三项。
 *  2. 调用 bsp_wrapper_xxx_register() 完成注册。
 *
 * @return void : 无返回值。
 * @note 单实例模板不使用 context，设备对象由文件内 static 维护。
 *
 */
void bsp_adapter_xxx_register(void)
{
    static const bsp_wrapper_xxx_dev_t dev =
    {
        .init = bsp_adapter_xxx_init,
        .control = bsp_adapter_xxx_control,
        .context = NULL
    };

    (void)bsp_wrapper_xxx_register(0U, &dev);
}
//************************* Function Definitions end *************************//

//************************** Static Functions begin **************************//
/**
 * @brief 初始化 XXX 的平台侧硬件资源。
 *
 * Steps:
 *  1. 检查 Port 上下文是否有效。
 *  2. 接入时在此补充时钟、GPIO 与外设初始化。
 *
 * @param[in] p_context : Port 上下文，本模板传入 Driver 实例地址。
 * @return bsp_xxx_status_t : 参数有效返回 BSP_XXX_OK，否则返回参数错误。
 *
 */
static bsp_xxx_status_t bsp_adapter_xxx_port_init(void *p_context)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_context)
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return BSP_XXX_OK;
}

/**
 * @brief 执行 XXX 的平台侧控制操作。
 *
 * Steps:
 *  1. 检查 Port 上下文是否有效。
 *  2. 按 command 分派具体控制动作，模板阶段直接返回成功。
 *
 * @param[in] p_context : Port 上下文，本模板传入 Driver 实例地址。
 * @param[in] command : 控制命令编码。
 * @param[in] p_argument : 命令参数指针，可为 NULL。
 * @return bsp_xxx_status_t : 参数有效返回 BSP_XXX_OK，否则返回参数错误。
 *
 */
static bsp_xxx_status_t bsp_adapter_xxx_port_control(
    void *p_context,
    uint32_t command,
    void *p_argument)
{
    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_context)
    {
        return BSP_XXX_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    (void)command;
    (void)p_argument;
    return BSP_XXX_OK;
}

/**
 * @brief 进入 XXX 的平台侧临界区。
 *
 * Steps:
 *  1. 接入时在此替换为关中断或 OS 临界区实现。
 *
 * @return bsp_xxx_status_t : 固定返回 BSP_XXX_OK。
 * @note 模板不做实际保护，移植时必须实现，否则并发访问不安全。
 *
 */
static bsp_xxx_status_t bsp_adapter_xxx_critical_enter(void)
{
    return BSP_XXX_OK;
}

/**
 * @brief 退出 XXX 的平台侧临界区。
 *
 * Steps:
 *  1. 接入时在此替换为开中断或 OS 退出临界区实现。
 *
 * @return bsp_xxx_status_t : 固定返回 BSP_XXX_OK。
 * @note 与 bsp_adapter_xxx_critical_enter() 成对调用。
 *
 */
static bsp_xxx_status_t bsp_adapter_xxx_critical_exit(void)
{
    return BSP_XXX_OK;
}

/**
 * @brief 把 Driver 状态码转换为 Wrapper 状态码。
 *
 * Steps:
 *  1. 按状态枚举逐项映射。
 *  2. 未列出的状态统一归入 BSP_WRAPPER_XXX_ERROR。
 *
 * @param[in] status : Driver 侧返回的状态。
 * @return bsp_wrapper_xxx_status_t : 对应的 Wrapper 状态。
 *
 */
static bsp_wrapper_xxx_status_t bsp_adapter_xxx_status_convert(
    bsp_xxx_status_t status)
{
    switch (status)
    {
        case BSP_XXX_OK:
            return BSP_WRAPPER_XXX_OK;
        case BSP_XXX_ERR_ARGUMENT:
            return BSP_WRAPPER_XXX_ERR_ARGUMENT;
        case BSP_XXX_ERR_STATE:
            return BSP_WRAPPER_XXX_ERR_STATE;
        default:
            return BSP_WRAPPER_XXX_ERROR;
    }
}

/**
 * @brief 初始化 XXX Driver。
 *
 * Steps:
 *  1. 把Adapter 组装的 Port 接口表传给 Driver inst。
 *  2. 调用 Driver inst，由 inst 保存资源、挂载函数并完成初始化。
 *
 * @param[in] p_context : Wrapper 透传的上下文，本模板不使用。
 * @return bsp_wrapper_xxx_status_t : Driver 初始化结果。
 * @note 无 Handler 的器件由 Adapter 持有 Driver 实例；
 *       有 Handler 时把本函数换成 bsp_xxx_handler_inst() 即可。
 *
 */
static bsp_wrapper_xxx_status_t bsp_adapter_xxx_init(void *p_context)
{
    (void)p_context;

    return bsp_adapter_xxx_status_convert(
        bsp_xxx_driver_inst(&g_bsp_adapter_xxx_driver,
                            &g_bsp_adapter_xxx_port_ops,
                            &g_bsp_adapter_xxx_driver));
}

/**
 * @brief 把 Wrapper 的控制命令转发给 XXX Driver。
 *
 * Steps:
 *  1. 检查命令参数是否有效。
 *  2. 调用 Driver 控制接口，并把状态码转为 Wrapper 状态。
 *
 * @param[in] p_context : Wrapper 透传的上下文，本模板不使用。
 * @param[in] index : 设备索引，单设备模板不使用。
 * @param[in] command : 控制命令编码。
 * @param[in] p_argument : 命令参数指针，可为 NULL。
 * @return bsp_wrapper_xxx_status_t : 控制操作状态。
 *
 */
static bsp_wrapper_xxx_status_t bsp_adapter_xxx_control(
    void *p_context,
    uint32_t index,
    uint32_t command,
    void *p_argument)
{
    //-------------------------参数检查 begin-------------------------//
    if ((false == g_bsp_adapter_xxx_driver.initialized) ||
        (NULL == g_bsp_adapter_xxx_driver.pf_control))
    {
        return BSP_WRAPPER_XXX_ERR_STATE;
    }
    //--------------------------参数检查 end--------------------------//

    (void)p_context;
    (void)index;

    return bsp_adapter_xxx_status_convert(
        g_bsp_adapter_xxx_driver.pf_control(
            &g_bsp_adapter_xxx_driver, command, p_argument));
}
//*************************** Static Functions end ***************************//

//******************************* End of File ********************************//
