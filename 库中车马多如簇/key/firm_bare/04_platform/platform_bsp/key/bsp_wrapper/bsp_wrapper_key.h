/******************************************************************************
 * Copyright (C) 2026 YA.
 * All Rights Reserved.
 * @file bsp_wrapper_key.h
 * @par dependencies
 * - ../../../05_bsp/bsp_key/bsp_key_types.h
 * - stdbool.h
 * - stdint.h
 * @author YA
 * @brief 保存多个按键器件的函数表，按"当前器件"转发 init / scan / notify。
 *        事件由 Adapter 在 scan 内部识别后 push 到 App 队列。
 * Processing flow:
 * Adapter reg(槽位, dev) -> init -> 主循环 scan -> ISR notify。
 * @version V1.0
 * @note 1 tab == 4 spaces!
 *****************************************************************************/
#ifndef BSP_WRAPPER_KEY_H
#define BSP_WRAPPER_KEY_H

//****************************** Includes begin ******************************//
#include "../../../05_bsp/bsp_key/bsp_key_types.h"
#include <stdbool.h>
#include <stdint.h>
//******************************* Includes end *******************************//

//******************************* Defines begin ******************************//
/* 允许同时存在的按键器件数量；reg 时的槽位上限。 */
#define BSP_WRAPPER_KEY_DRV_MAX_NUM   2U
//******************************** Defines end *******************************//

//****************************** Typedefs begin ******************************//
/* 按键器件函数表：一个 Adapter 填一份注册进来。 */
typedef struct bsp_wrapper_key_dev
{
    uint32_t u32_idx;         /* 槽位索引（reg 时写入）。 */
    void    *p_user_data;     /* Adapter 自管的上下文。 */

    /* 启动阶段挂载（内部挂 Handler/Driver 等）。 */
    en_key_status_t (*init)(struct bsp_wrapper_key_dev *p_dev);

    /* 主循环定时扫描；长按/短按识别后由 Adapter 推送到 App 队列。 */
    en_key_status_t (*scan)(struct bsp_wrapper_key_dev *p_dev);

    /* ISR 按 EXTI 引脚通知对应按键（内部置标志）。 */
    en_key_status_t (*notify)(struct bsp_wrapper_key_dev *p_dev,
        uint16_t u16_pin);
} bsp_wrapper_key_dev_t;
//******************************* Typedefs end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 注册器件函数表到指定槽位，并把当前器件切到它。
 * @param[in] u32_index 槽位索引，范围 0 ~ BSP_WRAPPER_KEY_DRV_MAX_NUM-1。
 * @param[in] p_dev 器件函数表。
 * @return bool 成功 true；参数无效或槽位已占用返回 false。
 */
bool bsp_wrapper_key_reg(uint32_t u32_index, bsp_wrapper_key_dev_t *p_dev);

/**
 * @brief 调用当前器件的 init。
 */
en_key_status_t bsp_wrapper_key_init(void);

/**
 * @brief 调用当前器件的 scan，供主循环定时触发。
 */
en_key_status_t bsp_wrapper_key_scan(void);

/**
 * @brief ISR 中按 EXTI 引脚通知对应按键。
 * @param[in] u16_pin 单一 EXTI 引脚掩码。
 */
en_key_status_t bsp_wrapper_key_notify(uint16_t u16_pin);
//************************* Function Prototypes end **************************//

#endif /* BSP_WRAPPER_KEY_H */
//******************************* End of File ********************************//
