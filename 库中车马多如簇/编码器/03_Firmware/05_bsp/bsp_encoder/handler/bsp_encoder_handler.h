/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_encoder_handler.h
 *
 * @par dependencies
 * - bsp_encoder_driver.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 提供 ENCODER Handler 的实例化和控制接口。
 *
 * Processing flow:
 *
 * Adapter 提供资源 -> Handler 调用 Driver inst -> Handler 转发操作。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef FIRMWARE_BSP_ENCODER_HANDLER_H
#define FIRMWARE_BSP_ENCODER_HANDLER_H

//****************************** Includes begin ******************************//
#include "../driver/bsp_encoder_driver.h"
#include <stdint.h>
//******************************* Includes end *******************************//

//******************************* Enums begin ********************************//
typedef enum
{
    BSP_ENCODER_CMD_READ_ANGLE = 0,
    BSP_ENCODER_CMD_READ_RAW_ANGLE,
    BSP_ENCODER_CMD_SET_RANGE,
    BSP_ENCODER_CMD_SET_POWER_MODE,
    BSP_ENCODER_CMD_SAVE_CONFIG,
    BSP_ENCODER_CMD_SET_FILTER,
    BSP_ENCODER_CMD_START_DMA,
    BSP_ENCODER_CMD_STOP_DMA,
    BSP_ENCODER_CMD_GET_DMA_ANGLE
} bsp_encoder_command_t;
//******************************** Enums end *********************************//

//****************************** Structs begin *******************************//
typedef struct
{
    uint16_t u16_start;
    uint16_t u16_stop;
} bsp_encoder_range_t;

typedef struct
{
    bsp_encoder_power_mode_t mode;
    bool watchdog_enabled;
} bsp_encoder_power_config_t;

typedef struct
{
    bsp_encoder_slow_filter_t slow_filter;
    bsp_encoder_fast_filter_t fast_filter;
    bsp_encoder_hysteresis_t hysteresis;
} bsp_encoder_filter_t;

typedef struct
{
    bsp_encoder_driver_t *p_drivers;
    uint32_t u32_device_count;
} bsp_encoder_handler_t;
//******************************* Structs end ********************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 检查全部设备配置后挂载 Driver 数组，成功前不更新 Handler。
 * @param[in,out] p_handler ENCODER Handler 对象。
 * @param[in,out] p_drivers 连续的 Driver 数组，长度至少为设备数量。
 * @param[in] u32_device_count 设备数量，必须大于 0。
 * @param[in] p_iic_interfaces 等长的 I2C 配置数组，与 Driver 一一对应。
 * @param[in] pf_delay_ms 所有设备共用的毫秒延时接口。
 * @return bsp_encoder_status_t : 实例化结果。
 * @note 调用前将 Handler 和 Driver 清零，先配置平台 I2C 端口。
 *       Driver 数组须在 Handler 使用期间保持有效，接口配置按值复制。
 *       AS5600 地址固定，多设备须使用独立总线或由平台切换通道。
 *       本函数不访问芯片，不设置寄存器，不执行 OTP 烧录。
 */
bsp_encoder_status_t bsp_encoder_handler_inst(
    bsp_encoder_handler_t *p_handler,
    bsp_encoder_driver_t *p_drivers,
    uint32_t u32_device_count,
    const bsp_encoder_iic_interface_t *p_iic_interfaces,
    bsp_encoder_delay_ms_t pf_delay_ms);

/**
 * @brief 检查命令并同步调用对应的 Driver 功能。
 * @param[in] p_handler ENCODER Handler 对象。
 * @param[in] index 设备索引，范围为 0 至 u32_device_count - 1。
 * @param[in] command bsp_encoder_command_t 中定义的命令。
 * @param[in,out] p_argument 命令对应的参数，不可为空。
 * @return bsp_encoder_status_t : 参数、未挂载或 Driver 操作结果。
 * @note READ_ANGLE/READ_RAW_ANGLE：uint16_t *，失败时不更新角度。
 *       SET_RANGE：bsp_encoder_range_t *，起止值为 0~4095。
 *       SET_POWER_MODE：bsp_encoder_power_config_t *。
 *       SAVE_CONFIG：bsp_encoder_save_t *，仅此命令执行 OTP 烧录。
 *       SET_FILTER：bsp_encoder_filter_t *，只写易失 CONF 滤波位。
 *       参数须使用对应类型并正确对齐；调用期间保持有效。
 *       同一 Driver 和总线须由上层串行访问。
 *       SAVE_CONFIG 的供电、次数及失败处理要求见 Driver 声明。
 */
bsp_encoder_status_t bsp_encoder_handler_control(
    bsp_encoder_handler_t *p_handler,
    uint32_t index,
    uint32_t command,
    void *p_argument);
//************************* Function Prototypes end **************************//

#endif /* FIRMWARE_BSP_ENCODER_HANDLER_H */

//******************************* End of File ********************************//
