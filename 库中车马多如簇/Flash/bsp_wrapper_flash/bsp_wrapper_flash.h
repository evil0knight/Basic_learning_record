/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_wrapper_flash.h
 *
 * @par dependencies
 * - stdint.h
 * - stdbool.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 外部 Flash Wrapper 层接口（函数表注册模式）。
 *
 * Processing flow:
 *
 * 1. Adapter 实现 externflash_drv_t 函数表;
 * 2. 调 bsp_wrapper_flash_reg(index, &drv) 注册;
 * 3. 上层统一调 bsp_wrapper_flash_read/write/erase 等接口。
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef BSP_WRAPPER_FLASH_H
#define BSP_WRAPPER_FLASH_H

//********************************Includes*********************************//

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

//********************************Includes*********************************//


/* 外部 Flash 驱动函数表（由 Adapter 填充并注册） */
typedef struct _externflash_drv_t
{
    uint32_t idx;      /* 实例索引 */
    uint32_t dev_id;   /* 器件 id */
    void *user_data;   /* 用户数据 */

    void (*externflash_drv_init)(struct _externflash_drv_t *dev);
    void (*externflash_drv_deinit)(struct _externflash_drv_t *dev);
    void (*externflash_drv_read)(struct _externflash_drv_t *dev,
                                 uint32_t addr, uint32_t size, uint8_t *p_buf);
    void (*externflash_drv_write)(struct _externflash_drv_t *dev,
                                  uint32_t addr, uint32_t size,
                                  uint8_t *p_buf);
    void (*externflash_drv_write_noerase)(struct _externflash_drv_t *dev,
                                          uint32_t addr, uint32_t size,
                                          uint8_t *p_buf);
    void (*externflash_drv_erasechip)(struct _externflash_drv_t *dev);
    void (*externflash_drv_erasesector)(struct _externflash_drv_t *dev,
                                        uint32_t addr);
    void (*externflash_drv_wakeup)(struct _externflash_drv_t *dev);
    void (*externflash_drv_sleep)(struct _externflash_drv_t *dev);
    void (*externflash_drv_test)(struct _externflash_drv_t *dev);
} externflash_drv_t;

/**
 * @brief 注册一个外部 Flash 驱动函数表。
 *
 * @param[in] index : 实例索引（< EXTERNFLASH_DRV_MAX_NUM）。
 * @param[in] dev   : 驱动函数表指针。
 *
 * @return bool : true 成功, false 参数非法。
 */
bool bsp_wrapper_flash_reg(uint32_t index, externflash_drv_t *dev);

/**
 * @brief 调用当前驱动的 init。
 */
void bsp_wrapper_flash_init(void);

/**
 * @brief 调用当前驱动的 deinit。
 */
void bsp_wrapper_flash_deinit(void);

/**
 * @brief 调用当前驱动的 read。
 *
 * @param[in]  addr  : 读起始地址。
 * @param[in]  size  : 读取长度。
 * @param[out] p_buf : 读缓冲。
 */
void bsp_wrapper_flash_read(uint32_t addr, uint32_t size, uint8_t *p_buf);

/**
 * @brief 调用当前驱动的 write（先擦后写）。
 */
void bsp_wrapper_flash_write(uint32_t addr, uint32_t size, uint8_t *p_buf);

/**
 * @brief 调用当前驱动的 write（不擦除）。
 */
void bsp_wrapper_flash_write_noerase(uint32_t addr, uint32_t size,
                                     uint8_t *p_buf);

/**
 * @brief 调用当前驱动的整片擦除。
 */
void bsp_wrapper_flash_erasechip(void);

/**
 * @brief 调用当前驱动的扇区擦除。
 *
 * @param[in] addr : 扇区起始地址。
 */
void bsp_wrapper_flash_erasesector(uint32_t addr);

/**
 * @brief 调用当前驱动的 wakeup。
 */
void bsp_wrapper_flash_wakeup(void);

/**
 * @brief 调用当前驱动的 sleep。
 */
void bsp_wrapper_flash_sleep(void);

/**
 * @brief 调用当前驱动的自检查。
 */
void bsp_wrapper_flash_test(void);

#endif /* BSP_WRAPPER_FLASH_H */
