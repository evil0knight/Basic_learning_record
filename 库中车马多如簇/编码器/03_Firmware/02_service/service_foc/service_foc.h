/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file service_foc.h
 *
 * @par dependencies
 * - ../../01_app/app_control/app_control.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 声明 FOC 服务层入口，当前先提供编码器角度采样。
 *
 * Processing flow:
 *
 * init 挂载编码器 -> 20kHz节拍异步采样 -> 上层读取缓存角度。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

#ifndef SERVICE_FOC_H
#define SERVICE_FOC_H

//****************************** Includes begin ******************************//
#include "../../01_app/app_control/app_control.h"
#include <stdint.h>
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 初始化 FOC 服务并挂载编码器。
 *
 * @return app_status_t : 成功返回 APP_STATUS_OK，否则返回底层状态。
 * @note 调用前须先注册编码器 Adapter。
 */
app_status_t service_foc_init(void);

/**
 * @brief 周期轮询 FOC 服务，当前只维护初始化状态。
 *
 * @param[in] u32_now_ms : 当前毫秒时间。
 *
 * @return app_status_t : 成功或初始化状态。
 * @note 在主循环中调用；编码器采样在 20kHz 节拍中完成。
 */
app_status_t service_foc_poll(uint32_t u32_now_ms);

/**
 * @brief 20kHz 控制节拍中的编码器采样与消费。
 *
 * @return void : 无返回值。
 * @note 由 HRTIM 比较中断调用；每 4 个 tick 启动一笔 DMA。
 */
void service_foc_encoder_tick(void);

/**
 * @brief 获取最近一次缓存的 12 位角度计数。
 *
 * @param[out] p_u16_angle : 12 位角度计数，范围 0~4095。
 *
 * @return app_status_t : 成功或参数错误。
 */
app_status_t service_foc_get_angle_count(uint16_t *p_u16_angle);

/**
 * @brief 把 12 位角度计数转换为机械角度。
 *
 * @param[in] u16_count : 12 位角度计数，范围 0~4095。
 *
 * @return float : 机械角度，单位 rad，范围 0~2π。
 * @note 按编码器默认整圈 4096 计数换算。
 */
float service_foc_count_to_mech_rad(uint16_t u16_count);
//************************* Function Prototypes end **************************//

#endif /* SERVICE_FOC_H */

//******************************* End of File ********************************//
