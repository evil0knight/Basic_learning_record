/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_encoder_driver_reg.h
 *
 * @par dependencies
 * - 无
 *
 * @author YA
 *
 * @brief 定义 AS5600 设备地址、寄存器地址、位掩码和 OTP 命令。
 *
 * Processing flow:
 *
 * 由 Driver 头文件包含，供寄存器访问和参数检查使用。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef FIRMWARE_BSP_ENCODER_DRIVER_REG_H
#define FIRMWARE_BSP_ENCODER_DRIVER_REG_H

//****************************** Defines begin *******************************//
/* I2C 设备地址 */
#define BSP_ENCODER_IIC_ADDRESS   (0x36U)   // 未左移的 7 位设备地址

/* 寄存器地址 */
#define ENCODER_REG_ZMCO          (0x00U)   // 角度 OTP 烧录次数寄存器
#define ENCODER_REG_ZPOS          (0x01U)   // 起始位置寄存器高字节地址
#define ENCODER_REG_MPOS          (0x03U)   // 终止位置寄存器高字节地址
#define ENCODER_REG_MANG          (0x05U)   // 最大角度范围寄存器高字节地址
#define ENCODER_REG_CONF          (0x07U)   // 配置寄存器高字节地址
#define ENCODER_REG_STATUS        (0x0BU)   // 磁铁检测状态寄存器
#define ENCODER_REG_RAW_ANGLE     (0x0CU)   // 原始角度寄存器高字节地址
#define ENCODER_REG_ANGLE         (0x0EU)   // 处理后角度寄存器高字节地址
#define ENCODER_REG_BURN          (0xFFU)   // OTP 烧录及重载命令寄存器

/* CONF 寄存器位 */
#define ENCODER_CONF_PM_MASK      (0x0003U) // CONF 电源模式位掩码
#define ENCODER_CONF_HYST_MASK    (0x000CU) // CONF 迟滞位掩码
#define ENCODER_CONF_HYST_SHIFT   (2U)      // CONF 迟滞位起始位
#define ENCODER_CONF_SF_MASK      (0x0300U) // CONF 慢滤波位掩码
#define ENCODER_CONF_SF_SHIFT     (8U)      // CONF 慢滤波位起始位
#define ENCODER_CONF_FTH_MASK     (0x1C00U) // CONF 快滤波阈值位掩码
#define ENCODER_CONF_FTH_SHIFT    (10U)     // CONF 快滤波阈值位起始位
#define ENCODER_CONF_WD_MASK      (0x2000U) // CONF 低功耗看门狗使能位

/* STATUS 寄存器位 */
#define ENCODER_STATUS_MD_MASK    (0x20U)   // STATUS 磁铁已检测标志位

/* ZMCO 寄存器位 */
#define ENCODER_ZMCO_MASK         (0x03U)   // 角度 OTP 烧录次数有效位掩码

/* 角度有效位和范围 */
#define ENCODER_ANGLE_MASK        (0x0FFFU) // 12 位角度有效位掩码
#define BSP_ENCODER_ANGLE_MAX     (4095U)   // 12 位角度计数最大值
#define ENCODER_MIN_RANGE         (205U)    // 大于 18 度的最小范围计数
#define ENCODER_COUNTS_PER_TURN   (4096U)   // 每圈角度计数总数

/* OTP 烧录命令 */
#define ENCODER_BURN_ANGLE        (0x80U)   // 烧录 ZPOS 和 MPOS 的命令
#define ENCODER_BURN_SETTING      (0x40U)   // 烧录 MANG 和 CONF 的命令
//******************************* Defines end ********************************//

#endif /* FIRMWARE_BSP_ENCODER_DRIVER_REG_H */

//******************************* End of File ********************************//
