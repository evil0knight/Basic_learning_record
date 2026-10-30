/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file bsp_key_driver.h
 *
 * @par dependencies
 * - ../bsp_key_types.h
 * - stdbool.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 提供按键 Driver 的实例化、GPIO 资源和读取操作。
 *
 * Processing flow:
 *
 * 组装 GPIO 接口 -> Driver inst 保存资源 -> Handler 调用读取操作。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#ifndef BSP_KEY_DRIVER_H
#define BSP_KEY_DRIVER_H

//****************************** Includes begin ******************************//
#include "../bsp_key_types.h"
#include <stdbool.h>
#include <stdint.h>
//******************************* Includes end *******************************//

//****************************** Typedefs begin ******************************//
typedef struct bsp_key_driver bsp_key_driver_t;
typedef struct
{
    en_key_status_t (*pf_read_level)(uint32_t u32_key_id, bool *p_high);
} bsp_key_port_ops_t;
typedef struct
{
    uint32_t u32_key_id;
    uint32_t u32_scan_ms;
    uint32_t u32_debounce_ms;
    uint32_t u32_long_press_ms;
} bsp_key_config_t;
typedef en_key_status_t (*bsp_key_read_pressed_t)(
    bsp_key_driver_t *p_driver, bool *p_pressed);
//******************************* Typedefs end *******************************//

//****************************** Structs begin *******************************//
struct bsp_key_driver
{
    const bsp_key_port_ops_t *p_port_ops;
    uint32_t u32_key_id;
    bool b_active_high;
    bool b_initialized;
    bsp_key_read_pressed_t pf_read_pressed;
    bsp_key_config_t st_config;
    volatile bool b_pending;
    bool b_active;
    bool b_candidate_pressed;
    bool b_stable_pressed;
    bool b_long_reported;
    bool b_irq_attached;
    uint32_t u32_last_scan_ms;
    uint32_t u32_candidate_since_ms;
    uint32_t u32_press_since_ms;
};
//******************************* Structs end ********************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 实例化按键 Driver，保存按键 ID 并挂载读取操作。
 *
 * @param[in,out] p_driver : 按键 Driver 对象。
 * @param[in] p_port_ops : GPIO 平台接口表。
 * @param[in] u32_key_id : 本键在 Adapter 的 pin 表索引，read_level/irq_attach 透传。
 * @param[in] b_active_high : true 表示高电平按下。
 * @return en_key_status_t : 实例化结果。
 */
en_key_status_t bsp_key_driver_inst(
    bsp_key_driver_t *p_driver,
    const bsp_key_port_ops_t *p_port_ops,
    uint32_t u32_key_id,
    bool b_active_high);

/**
 * @brief 根据有效电平读取按键按下状态。
 * @param[in] p_driver : 按键 Driver 对象。
 * @param[out] p_pressed : 按键按下状态。
 * @return en_key_status_t : 读取结果。
 */
en_key_status_t bsp_key_driver_read_pressed(
    bsp_key_driver_t *p_driver, bool *p_pressed);

/**
 * @brief 清除按键 Driver 实例状态。
 * @param[in,out] p_driver : 按键 Driver 对象。
 * @return en_key_status_t : 清除结果。
 */
en_key_status_t bsp_key_driver_deinit(bsp_key_driver_t *p_driver);
//************************* Function Prototypes end **************************//

#endif /* BSP_KEY_DRIVER_H */
//******************************* End of File ********************************//
