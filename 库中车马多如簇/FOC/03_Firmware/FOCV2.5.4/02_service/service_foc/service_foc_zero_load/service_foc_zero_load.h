/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file service_foc_zero_load.h
 *
 * @par dependencies
 * - ../../../01_app/app_control/app_control.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 电角度零点标定与零偏存取接口。
 *
 * Processing flow:
 *
 * 加载零偏 -> 主循环对齐与验证 -> 关闭输出 -> 保存零偏。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef SERVICE_FOC_ZERO_LOAD_H
#define SERVICE_FOC_ZERO_LOAD_H

//****************************** Includes begin ******************************//
#include "../../../01_app/app_control/app_control.h"
#include <stdint.h>
//******************************* Includes end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 输出已关闭时阻塞确认电机 1 连续静止及衰减等待完成。
 * @return app_status_t : 成功、参数错误或读取/停稳超时错误。
 * @note 仅主循环调用，暂停 FOC 回调；成功才标记已确认停止。
 *       尚无电流零偏时只观察编码器，衰减按现有等待时间判断。
 */
app_status_t service_foc_motor1_wait_stop(void);


/**
 * @brief 保存电角度零偏和编码器方向到 Flash 并读回验证。
 *
 * Steps:
 *  1. 校验编号、角度及方向，构造带方向标识和反码的 16 字节记录。
 *  2. 写入本电机记录并读回比较，失败时返回 IO 错误。
 *
 * @param[in] u8_motor_id : 电机编号，1 或 2。
 * @param[in] f_zero_deg : 电角度零偏，范围 [0, 360)，单位度。
 * @param[in] i8_direction : 自动标定的编码器方向，+1 或 -1。
 * @return app_status_t : 成功、参数错误或 Flash IO 错误。
 * @note 仅在主循环串行调用，所有电机输出须关闭；不保证掉电原子性。
 */
app_status_t service_foc_zero_save(uint8_t u8_motor_id, float f_zero_deg,
    int8_t i8_direction);

/**
 * @brief 加载零偏及方向；旧版或无效记录保留默认值并标记未标定。
 *
 * Steps:
 *  1. 读取对应电机记录，检查版本标识、反码和角度范围。
 *  2. 仅有效记录覆盖输出；空白或损坏记录保留调用者的默认值。
 *
 * @param[in] u8_motor_id : 电机编号，1 或 2。
 * @param[in,out] p_zero_deg : 配置默认值，成功加载后为 Flash 值。
 * @param[in,out] p_direction : 默认方向，成功加载后为标定方向。
 * @param[out] p_valid : 1 为有效记录，空白/损坏/读取失败为 0。
 * @return app_status_t : 成功、参数错误或读取失败。
 */
app_status_t service_foc_zero_load(uint8_t u8_motor_id,
    float *p_zero_deg, int8_t *p_direction, uint8_t *p_valid);

/**
 * @brief 查询电机 1 是否正在进行零点标定。
 *
 * Steps:
 *  1. 检查输出指针并读取标定忙标志。
 *
 * @param[out] p_calibrating : 0 表示空闲，1 表示正在标定。
 * @return app_status_t : 成功或参数错误。
 */
app_status_t service_foc_zero_get_calibrating(uint8_t *p_calibrating);

/**
 * @brief 在主循环阻塞标定电机 1 电角度零偏，期间暂停正常 FOC。
 *
 * Steps:
 *  1. 阻塞停机并校准电流零偏，再施加零度电压矢量。
 *  2. 保持矢量，采集编码器并以圆周平均求机械零点。
 *  3. 改为正 90 度矢量，验证位移幅度并自动识别方向。
 *  4. 关闭 PWM，验证成功后将零偏和方向一起保存到 Flash 和 RAM。
 *  5. 流程结束前向事件队列投递 EVENT_CALIBRATION_EXIT。
 *
 * @return app_status_t : 成功、参数错误或采样/输出/对齐验证失败。
 * @note 只在硬件初始化后、全局中断开启的主循环调用，先关断并等停稳。
 *       幅值和时间在函数开头修改；成功后写入 Flash，不自动恢复输出。
 *       PWM 比较值映射须实测确认，固定电压不代表闭环恒流。
 *       退出事件由本函数负责投递，调用方无需再处理退出流程。
 */
app_status_t service_foc_motor1_calibrate(void);

//************************* Function Prototypes end **************************//

#endif /* SERVICE_FOC_ZERO_LOAD_H */
