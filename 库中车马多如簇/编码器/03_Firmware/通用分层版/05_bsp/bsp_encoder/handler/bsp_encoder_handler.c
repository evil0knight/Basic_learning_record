/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_encoder_handler.c
 *
 * @par dependencies
 * - bsp_encoder_handler.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 实现 ENCODER Handler 的实例化和控制转发。
 *
 * Processing flow:
 *
 * 参数检查 -> 调用 Driver inst -> 保存 Driver -> 转发操作。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
//****************************** Includes begin ******************************//
#include "bsp_encoder_handler.h"
#include <stddef.h>
//******************************* Includes end *******************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 检查所有设备配置，逐个调用 Driver inst 后保存数组和数量。
 * @param[in,out] p_handler ENCODER Handler 对象。
 * @param[in,out] p_drivers 连续的 Driver 数组。
 * @param[in] u32_device_count 数组中的设备数量。
 * @param[in] p_iic_interfaces 与 Driver 一一对应的 I2C 配置数组。
 * @param[in] pf_delay_ms 所有设备共用的毫秒延时接口。
 * @return bsp_encoder_status_t : 实例化结果。
 */
bsp_encoder_status_t bsp_encoder_handler_inst(
    bsp_encoder_handler_t *p_handler,
    bsp_encoder_driver_t *p_drivers,
    uint32_t u32_device_count,
    const bsp_encoder_iic_interface_t *p_iic_interfaces,
    bsp_encoder_delay_ms_t pf_delay_ms)
{
    bsp_encoder_status_t status;
    uint32_t u32_index;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_handler) || (NULL == p_drivers) ||
        (0U == u32_device_count) || (NULL == p_iic_interfaces) ||
        (NULL == pf_delay_ms))
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    for (u32_index = 0U; u32_index < u32_device_count; u32_index++)
    {
        if ((NULL == p_iic_interfaces[u32_index].pf_mem_read) ||
            (NULL == p_iic_interfaces[u32_index].pf_mem_write) ||
            (CORE_I2C_PORT_MAX <= p_iic_interfaces[u32_index].port))
        {
            return BSP_ENCODER_ERR_ARGUMENT;
        }
    }
    //--------------------------参数检查 end--------------------------//

    for (u32_index = 0U; u32_index < u32_device_count; u32_index++)
    {
        status = bsp_encoder_driver_inst(&p_drivers[u32_index],
            &p_iic_interfaces[u32_index], pf_delay_ms);
        if (BSP_ENCODER_OK != status)
        {
            return status;
        }
    }

    p_handler->p_drivers = p_drivers;
    p_handler->u32_device_count = u32_device_count;
    return BSP_ENCODER_OK;
}

/**
 * @brief 根据命令和参数同步调用 Driver 的具体功能。
 * @param[in] p_handler ENCODER Handler 对象。
 * @param[in] index 设备索引，必须小于已挂载的设备数量。
 * @param[in] command 控制命令。
 * @param[in,out] p_argument 对应命令类型的参数。
 * @return bsp_encoder_status_t : 操作结果。
 */
bsp_encoder_status_t bsp_encoder_handler_control(
    bsp_encoder_handler_t *p_handler,
    uint32_t index,
    uint32_t command,
    void *p_argument)
{
    bsp_encoder_driver_t *p_driver;
    const bsp_encoder_range_t *p_range;
    const bsp_encoder_power_config_t *p_power_config;
    const bsp_encoder_save_t *p_target;
    const bsp_encoder_filter_t *p_filter;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_handler) || (NULL == p_argument))
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    if ((NULL == p_handler->p_drivers) ||
        (0U == p_handler->u32_device_count))
    {
        return BSP_ENCODER_ERR_STATE;
    }
    if (p_handler->u32_device_count <= index)
    {
        return BSP_ENCODER_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    p_driver = &p_handler->p_drivers[index];
    switch (command)
    {
        case BSP_ENCODER_CMD_READ_ANGLE:     // 读取处理后的角度
            return bsp_encoder_read_angle(p_driver,
                                          (uint16_t *)p_argument);

        case BSP_ENCODER_CMD_READ_RAW_ANGLE: // 读取原始角度
            return bsp_encoder_read_raw_angle(p_driver,
                                              (uint16_t *)p_argument);

        case BSP_ENCODER_CMD_SET_RANGE:      // 设置测量起止范围
            p_range = (const bsp_encoder_range_t *)p_argument;
            return bsp_encoder_set_range(p_driver,
                p_range->u16_start, p_range->u16_stop);

        case BSP_ENCODER_CMD_SET_POWER_MODE: // 设置电源模式和看门狗
            p_power_config = (const bsp_encoder_power_config_t *)p_argument;
            return bsp_encoder_set_power_mode(p_driver,
                p_power_config->mode, p_power_config->watchdog_enabled);

        case BSP_ENCODER_CMD_SAVE_CONFIG:    // 烧录选定的 OTP 配置
            p_target = (const bsp_encoder_save_t *)p_argument;
            return bsp_encoder_save_config(p_driver, *p_target);

        case BSP_ENCODER_CMD_SET_FILTER:     // 设置慢/快滤波和迟滞
            p_filter = (const bsp_encoder_filter_t *)p_argument;
            return bsp_encoder_set_filter(p_driver,
                p_filter->slow_filter, p_filter->fast_filter,
                p_filter->hysteresis);

        case BSP_ENCODER_CMD_START_DMA:      // 启动一笔异步角度采样
            return bsp_encoder_start_angle_dma(p_driver);

        case BSP_ENCODER_CMD_STOP_DMA:       // 停止异步角度采样
            return bsp_encoder_stop_angle_dma(p_driver);

        case BSP_ENCODER_CMD_GET_DMA_ANGLE:  // 从 AB 缓冲取最新角度
            return bsp_encoder_get_angle_dma(p_driver,
                                             (uint16_t *)p_argument);

        default:                             // 拒绝未定义的命令
            return BSP_ENCODER_ERR_ARGUMENT;
    }
}
//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
