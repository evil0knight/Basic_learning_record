/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_encoder_driver.h
 *
 * @par dependencies
 * - bsp_encoder_driver_reg.h
 * - i2c_port_types.h
 * - stdbool.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief AS5600 角度读取、范围、电源和 OTP 保存接口。
 *
 * Processing flow:
 *
 * inst 挂载底层接口 -> 直接调用功能函数 -> I2C 寄存器操作。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef FIRMWARE_BSP_ENCODER_DRIVER_H
#define FIRMWARE_BSP_ENCODER_DRIVER_H

//****************************** Includes begin ******************************//
#include "bsp_encoder_driver_reg.h"
#include "../../../04_platform/platform_mcu/i2c/bus/inc/i2c_port_types.h"
#include "../../../06_middlewares/buffer/ab_buffer.h"
#include <stdbool.h>
#include <stdint.h>
//******************************* Includes end *******************************//

//******************************* Enums begin ********************************//
typedef enum
{
    BSP_ENCODER_OK = 0,
    BSP_ENCODER_ERROR = -1,
    BSP_ENCODER_ERR_ARGUMENT = -2,
    BSP_ENCODER_ERR_STATE = -3,
    BSP_ENCODER_ERR_VERIFY = -4,
    BSP_ENCODER_ERR_TIMEOUT = -5,
    BSP_ENCODER_ERR_BUSY = -6,
    BSP_ENCODER_ERR_NACK = -7,
    BSP_ENCODER_ERR_EMPTY = -8
} bsp_encoder_status_t;

typedef enum
{
    BSP_ENCODER_POWER_NORMAL = 0,
    BSP_ENCODER_POWER_LPM1,
    BSP_ENCODER_POWER_LPM2,
    BSP_ENCODER_POWER_LPM3
} bsp_encoder_power_mode_t;

typedef enum
{
    BSP_ENCODER_SAVE_ANGLE = 0,
    BSP_ENCODER_SAVE_SETTING
} bsp_encoder_save_t;

typedef enum
{
    BSP_ENCODER_SLOW_FILTER_16X = 0,    /* 16x，阶跃 2.2ms，噪声最小 */
    BSP_ENCODER_SLOW_FILTER_8X,         /* 8x，阶跃 1.1ms */
    BSP_ENCODER_SLOW_FILTER_4X,         /* 4x，阶跃 0.55ms */
    BSP_ENCODER_SLOW_FILTER_2X          /* 2x，阶跃 0.286ms，噪声最大 */
} bsp_encoder_slow_filter_t;

typedef enum
{
    BSP_ENCODER_FAST_FILTER_SLOW_ONLY = 0,  /* 000：只用慢滤波 */
    BSP_ENCODER_FAST_FILTER_6LSB,           /* 001：阈值 6 LSB */
    BSP_ENCODER_FAST_FILTER_7LSB,           /* 010：阈值 7 LSB */
    BSP_ENCODER_FAST_FILTER_9LSB,           /* 011：阈值 9 LSB */
    BSP_ENCODER_FAST_FILTER_18LSB,          /* 100：阈值 18 LSB */
    BSP_ENCODER_FAST_FILTER_21LSB,          /* 101：阈值 21 LSB */
    BSP_ENCODER_FAST_FILTER_24LSB,          /* 110：阈值 24 LSB */
    BSP_ENCODER_FAST_FILTER_10LSB           /* 111：阈值 10 LSB */
} bsp_encoder_fast_filter_t;

typedef enum
{
    BSP_ENCODER_HYSTERESIS_OFF = 0,     /* 00：关闭迟滞 */
    BSP_ENCODER_HYSTERESIS_1LSB,        /* 01：1 LSB */
    BSP_ENCODER_HYSTERESIS_2LSB,        /* 10：2 LSB */
    BSP_ENCODER_HYSTERESIS_3LSB         /* 11：3 LSB */
} bsp_encoder_hysteresis_t;
//******************************** Enums end *********************************//

//****************************** Typedefs begin ******************************//
/**
 * @brief 同步读取寄存器，成功前需完成全部数据传输。
 * @param[in] port : 平台逻辑 I2C 端口号。
 * @param[in] u8_address : 未左移的 7 位设备地址。
 * @param[in] u16_register : 起始寄存器地址。
 * @param[in] address_size : 寄存器地址宽度。
 * @param[out] p_data : 接收缓冲区，高字节在前。
 * @param[in] u16_size : 读取字节数。
 * @param[in] u32_timeout_ms : 传输超时，单位 ms。
 * @return en_core_i2c_status_t : 平台传输结果。
 * @note 一次调用完成寄存器寻址及连续读取，不保存缓冲区指针。
 */
typedef en_core_i2c_status_t (*bsp_encoder_iic_mem_read_t)(
    core_i2c_port_id_t port, uint8_t u8_address,
    uint16_t u16_register, en_core_i2c_mem_addr_size_t address_size,
    uint8_t *p_data, uint16_t u16_size, uint32_t u32_timeout_ms);

/**
 * @brief 同步写入寄存器，成功前需完成全部数据传输。
 * @param[in] port : 平台逻辑 I2C 端口号。
 * @param[in] u8_address : 未左移的 7 位设备地址。
 * @param[in] u16_register : 起始寄存器地址。
 * @param[in] address_size : 寄存器地址宽度。
 * @param[in] p_data : 发送缓冲区，高字节在前。
 * @param[in] u16_size : 写入字节数。
 * @param[in] u32_timeout_ms : 传输超时，单位 ms。
 * @return en_core_i2c_status_t : 平台传输结果。
 * @note 不保存缓冲区指针。
 */
typedef en_core_i2c_status_t (*bsp_encoder_iic_mem_write_t)(
    core_i2c_port_id_t port, uint8_t u8_address,
    uint16_t u16_register, en_core_i2c_mem_addr_size_t address_size,
    const uint8_t *p_data, uint16_t u16_size, uint32_t u32_timeout_ms);

/**
 * @brief 提交一次异步 I2C 请求。
 * @param[in] p_request : 已填好的 I2C 请求。
 * @return en_core_i2c_status_t : 启动结果。
 */
typedef en_core_i2c_status_t (*bsp_encoder_iic_submit_t)(
    core_i2c_request_t *p_request);

/**
 * @brief 等待至少指定的毫秒数。
 * @param[in] u32_ms : 等待时间，单位 ms。
 * @return bsp_encoder_status_t : 延时结果。
 */
typedef bsp_encoder_status_t (*bsp_encoder_delay_ms_t)(uint32_t u32_ms);

//******************************* Typedefs end *******************************//

//****************************** Structs begin *******************************//
/**
 * @brief 编码器 I2C 接口，读写签名与平台寄存器访问函数一致。
 * @note port 对应已配置的平台槽位；超时值原样传给平台。
 *       AS5600 的设备地址和 8 位寄存器地址宽度由驱动提供。
 */
typedef struct
{
    bsp_encoder_iic_mem_read_t pf_mem_read;
    bsp_encoder_iic_mem_write_t pf_mem_write;
    bsp_encoder_iic_submit_t pf_submit;
    core_i2c_port_id_t port;
    uint32_t u32_timeout_ms;
} bsp_encoder_iic_interface_t;

typedef struct
{
    bsp_encoder_iic_interface_t iic_interface;
    bsp_encoder_delay_ms_t pf_delay_ms;

    uint8_t a_u8_dma_buf_a[2];
    uint8_t a_u8_dma_buf_b[2];
    ab_buffer_t s_angle_ab;
    core_i2c_request_t s_dma_req;
    volatile uint8_t u8_dma_enable;
    volatile uint8_t u8_dma_busy;
    volatile uint16_t u16_latest_angle;
} bsp_encoder_driver_t;
//******************************* Structs end ********************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 挂载底层接口，不初始化外设，不修改 AS5600 寄存器。
 * @param[out] p_driver : Driver 对象。
 * @param[in] p_iic_interface : I2C 接口及端口配置，不可为空。
 * @param[in] pf_delay_ms : 毫秒延时接口，不可为空。
 * @return bsp_encoder_status_t : 成功或参数错误。
 * @note 调用前先初始化 GPIO/I2C；同一对象和总线的访问由上层串行化。
 *       接口结构体按值复制；同步配置可挂载 core_i2c_mem_read/write，
 *       异步采样挂载 core_i2c_submit。
 */
bsp_encoder_status_t bsp_encoder_driver_inst(
    bsp_encoder_driver_t *p_driver,
    const bsp_encoder_iic_interface_t *p_iic_interface,
    bsp_encoder_delay_ms_t pf_delay_ms);

/**
 * @brief 读取经过范围缩放和滤波的 ANGLE 值。
 * @param[in] p_driver : 已挂载接口的 Driver 对象。
 * @param[out] p_u16_angle : 12 位计数值，范围 0~4095，失败时保持原值。
 * @return bsp_encoder_status_t : 参数、接口状态或传输结果。
 */
bsp_encoder_status_t bsp_encoder_read_angle(
    const bsp_encoder_driver_t *p_driver, uint16_t *p_u16_angle);

/**
 * @brief 读取未经范围缩放的 RAW ANGLE 值。
 * @param[in] p_driver : 已挂载接口的 Driver 对象。
 * @param[out] p_u16_angle : 0~4095，角度为计数值乘以 360/4096 度。
 * @return bsp_encoder_status_t : 参数、接口状态或传输结果。
 */
bsp_encoder_status_t bsp_encoder_read_raw_angle(
    const bsp_encoder_driver_t *p_driver, uint16_t *p_u16_angle);

/**
 * @brief 启动一笔角度异步 DMA 接收。
 * @param[in,out] p_driver : 已挂载接口的 Driver 对象。
 * @return bsp_encoder_status_t : 启动结果；上一笔在飞时返回 BUSY。
 * @note 使用 AB 双缓冲；每次完成后只提交当前块，
 *       下一次采样由外部固定节拍再次调用本函数。
 */
bsp_encoder_status_t bsp_encoder_start_angle_dma(
    bsp_encoder_driver_t *p_driver);

/**
 * @brief 停止角度异步 DMA 接收。
 * @param[in,out] p_driver : 已挂载接口的 Driver 对象。
 * @return bsp_encoder_status_t : 停止结果。
 * @note 允许一次在飞传输完成，但外部节拍不再启动下一次。
 */
bsp_encoder_status_t bsp_encoder_stop_angle_dma(
    bsp_encoder_driver_t *p_driver);

/**
 * @brief 从 AB 双缓冲取得最新角度计数。
 * @param[in,out] p_driver : 已挂载接口的 Driver 对象。
 * @param[out] p_u16_angle : 12 位角度计数，范围 0~4095。
 * @return bsp_encoder_status_t : 成功、空或参数错误。
 * @note 会消费所有已提交样本并保留最新值；没有新样本时返回 EMPTY。
 */
bsp_encoder_status_t bsp_encoder_get_angle_dma(
    bsp_encoder_driver_t *p_driver, uint16_t *p_u16_angle);

/**
 * @brief 设置 ZPOS/MPOS 测量范围，并清零 MANG 以采用起止位置模式。
 * @param[in] p_driver : 已挂载接口的 Driver 对象。
 * @param[in] u16_start : 起始原始角度计数，0~4095。
 * @param[in] u16_stop : 沿 DIR 规定方向的终止原始角度计数，0~4095。
 * @return bsp_encoder_status_t : 参数、接口状态或传输结果。
 * @note 相等表示整圈；否则跨度必须大于 18 度，即至少 205 个计数。
 *       只修改易失寄存器；多次写入中途失败时可能已部分生效。
 */
bsp_encoder_status_t bsp_encoder_set_range(
    const bsp_encoder_driver_t *p_driver,
    uint16_t u16_start, uint16_t u16_stop);

/**
 * @brief 设置电源模式和静止一分钟后进入 LPM3 的看门狗开关。
 * @param[in] p_driver : 已挂载接口的 Driver 对象。
 * @param[in] mode : 正常模式或 LPM1/LPM2/LPM3。
 * @param[in] watchdog_enabled : 是否启用低功耗看门狗。
 * @return bsp_encoder_status_t : 参数、接口状态或传输结果。
 * @note 保留 CONF 其他位，仅修改 PM 和 WD。
 */
bsp_encoder_status_t bsp_encoder_set_power_mode(
    const bsp_encoder_driver_t *p_driver,
    bsp_encoder_power_mode_t mode, bool watchdog_enabled);

/**
 * @brief 设置片内慢滤波强度、快滤波阈值和迟滞，保留 CONF 其他位。
 * @param[in] p_driver : 已挂载接口的 Driver 对象。
 * @param[in] slow_filter : 慢滤波强度，16x/8x/4x/2x。
 * @param[in] fast_filter : 快滤波阈值，SLOW_ONLY 表示只用慢滤波。
 * @param[in] hysteresis : 输出迟滞，0~3 LSB。
 * @return bsp_encoder_status_t : 参数、接口状态或传输结果。
 * @note 只修改易失 CONF 寄存器，掉电恢复默认；
 *       需永久生效须调用 bsp_encoder_save_config(BSP_ENCODER_SAVE_SETTING)。
 *       低功耗模式下慢滤波被芯片强制，SF 设置可能不生效。
 */
bsp_encoder_status_t bsp_encoder_set_filter(
    const bsp_encoder_driver_t *p_driver,
    bsp_encoder_slow_filter_t slow_filter,
    bsp_encoder_fast_filter_t fast_filter,
    bsp_encoder_hysteresis_t hysteresis);

/**
 * @brief 烧录选定 OTP 区域，等待完成后重载 OTP 并核对寄存器。
 * @param[in] p_driver : 已挂载接口的 Driver 对象。
 * @param[in] target : ANGLE 保存 ZPOS/MPOS，SETTING 保存 MANG/CONF。
 * @return bsp_encoder_status_t : 传输、延时结果或 ERR_VERIFY。
 * @note ANGLE 要求检测到磁铁且 ZMCO 小于 3，最多烧录三次。
 *       SETTING 仅允许烧录一次，调用者须确认此前未烧录过。
 *       ZMCO 非零时 MANG 无法烧录，本接口拒绝保存非零 MANG。
 *       烧录供电须满足手册第 20 页要求，3.3V 模式须至少 3.3V，
 *       VDD3V3 对地接 10uF；完成后还需重新上电验证。
 *       一旦发出烧录命令，即使后续失败，也不可直接自动重试。
 */
bsp_encoder_status_t bsp_encoder_save_config(
    const bsp_encoder_driver_t *p_driver, bsp_encoder_save_t target);
//************************* Function Prototypes end **************************//

#endif /* FIRMWARE_BSP_ENCODER_DRIVER_H */

//******************************* End of File ********************************//
