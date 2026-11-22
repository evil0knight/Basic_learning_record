/*****************************************************************************
 * Copyright (C) 2026 看门狗模块贡献者。
 *
 * 保留所有权利。
 *
 * @file watchdog_adapter_port.h
 *
 * @par dependencies
 * - stdint.h
 * - stdbool.h
 *
 * @author YA
 *
 * @brief 提供硬件看门狗、备份计数和系统复位适配接口。
 *
 * 处理流程：
 * 由上层初始化并调用接口，具体操作见各函数说明。
 *
 * @version V1.0
 *
 * @note 1 个制表符等于 4 个空格；注释采用 UTF-8 中文。
 *
 ****************************************************************************/

#ifndef WATCHDOG_ADAPTER_PORT_H
#define WATCHDOG_ADAPTER_PORT_H

//****************************** Includes begin ******************************//
#include <stdint.h>
#include <stdbool.h>
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//

/**
 * @brief 初始化 STM32F411 独立看门狗和备份域访问。
 *
 * 步骤：
 *  1. 使能 PWR 时钟，开放备份域写访问。
 *  2. 配置 IWDG 为 64 分频、重装值 2500，调用 HAL 初始化。
 *
 * @param 无。
 *
 * @return 无；当前实现未处理 HAL 初始化失败。
 *
 */
void watchdog_port_init(void);

/**
 * @brief 刷新独立看门狗计数器。
 *
 * 步骤：
 *  1. 调用 HAL 刷新已初始化的 IWDG。
 *
 * @param 无。
 *
 * @return 无；当前实现未检查 HAL 返回值。
 *
 */
void watchdog_port_refresh(void);

/**
 * @brief 读取保存的启动失败计数。
 *
 * 步骤：
 *  1. 读取 RTC 备份寄存器 BKP0R。
 *
 * @param 无。
 *
 * @return uint32_t：已保存的启动失败计数。
 *
 */
uint32_t watchdog_port_boot_fail_read(void);

/**
 * @brief 保存启动失败计数。
 *
 * 步骤：
 *  1. 开放备份域写访问，再写入 RTC 备份寄存器 BKP0R。
 *
 * @param[in] count : 待保存的启动失败计数。
 *
 * @return 无。
 *
 */
void watchdog_port_boot_fail_write(uint32_t count);

/**
 * @brief 判断本次启动是否带有 IWDG 复位标志。
 *
 * 步骤：
 *  1. 读取 IWDG 复位标志。
 *  2. 清除全部 RCC 复位标志，再返回读取结果。
 *
 * @param 无。
 *
 * @return bool：true 表示存在 IWDG 复位标志，否则为 false。
 *
 */
bool watchdog_port_is_wdg_reset(void);

/**
 * @brief 请求系统软件复位。
 *
 * 步骤：
 *  1. 调用 CMSIS 系统复位接口。
 *
 * @param 无。
 *
 * @return 无；正常执行复位后不返回。
 *
 */
void watchdog_port_system_reset(void);

//************************* Function Prototypes end **************************//

#endif

//******************************* End of File ********************************//
