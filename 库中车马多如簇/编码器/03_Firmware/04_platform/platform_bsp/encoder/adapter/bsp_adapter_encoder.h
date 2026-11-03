/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_adapter_encoder.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief 声明编码器 Adapter 的注册入口，供平台初始化流程调用。
 *
 * Processing flow:
 *
 * 目标工程初始化 -> bsp_adapter_encoder_register() -> Wrapper 取得操作表。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef BSP_ADAPTER_ENCODER_H
#define BSP_ADAPTER_ENCODER_H

//****************************** Includes begin ******************************//
#include <stdint.h>
//******************************* Includes end *******************************//

//******************************* Enums begin ********************************//
/* 板级编码器编号：数值即板级配置表 g_a_bsp_adapter_encoder_board 的行号。
 * 增加第二颗编码器时，在 _MAX 之前插入新编号，并同步配置表。 */
typedef enum
{
    BSP_ENCODER_ID_MOTOR = 0,   /* 电机端编码器 */
    BSP_ENCODER_ID_MAX
} bsp_encoder_id_t;
//******************************** Enums end *********************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 将编码器 Adapter 注册到 BSP Wrapper。
 *
 * Steps:
 *  1. 按板级配置表组装 {u32_idx, p_user_data, 7 个操作函数}。
 *  2. 逐台调用 bsp_wrapper_encoder_reg() 挂载函数表。
 *
 * @return void : 无返回值；注册结果由 Wrapper 内部记录。
 * @note 需在 Wrapper 首次被调用前注册一次，重复注册会被 Wrapper 拒绝。
 *       注册成功后由 bsp_wrapper_encoder_init() 触发 Handler 挂载。
 *
 */
void bsp_adapter_encoder_register(void);
//************************* Function Prototypes end **************************//

#endif /* BSP_ADAPTER_ENCODER_H */

//******************************* End of File ********************************//
