/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_wrapper_flash.c
 *
 * @par dependencies
 * - bsp_wrapper_flash.h
 *
 * @author YA
 *
 * @brief 外部 Flash Wrapper 实现: 纯函数表转发层, 无状态、无 OS 依赖。
 *
 * Processing flow:
 *
 * 保存上层注册的 externflash_drv_t 函数表,
 * 把 read/write/erase 等调用转发到当前驱动。
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes*********************************//

#include "bsp_wrapper_flash.h"

//********************************Includes*********************************//


#define EXTERNFLASH_DRV_MAX_NUM (2U)

/* 已注册驱动表 */
static externflash_drv_t _externflash_drv_dev[EXTERNFLASH_DRV_MAX_NUM];

/* 当前使用的驱动索引（最近一次注册的） */
static uint32_t _s_current_externflash_dev_idx = 0U;

/**
 * @brief 取当前生效的驱动实例指针。
 *
 * @return externflash_drv_t * : 当前驱动指针。
 */
static externflash_drv_t *get_current_externflash_drv(void)
{
    return &_externflash_drv_dev[_s_current_externflash_dev_idx];
}

/**
 * @brief 注册一个外部 Flash 驱动函数表。
 *
 * Steps:
 *  1. 参数检查: index 范围, dev 非空。
 *  2. 复制函数表字段到内部数组。
 *  3. 更新"当前驱动"索引。
 *
 * @param[in] index : 实例索引（< EXTERNFLASH_DRV_MAX_NUM）。
 * @param[in] dev   : 驱动函数表指针。
 *
 * @return bool : true 成功, false 参数非法。
 */
bool bsp_wrapper_flash_reg(uint32_t index, externflash_drv_t *dev)
{
    /* 参数检查: 越界或空指针直接拒绝 */
    if ((index >= EXTERNFLASH_DRV_MAX_NUM) || (NULL == dev))
    {
        return false;
    }

    /* 逐字段复制, 保证内部表与外部结构解耦 */
    _externflash_drv_dev[index].idx                           = index;
    _externflash_drv_dev[index].dev_id                        = dev->dev_id;
    _externflash_drv_dev[index].user_data                     = dev->user_data;
    _externflash_drv_dev[index].externflash_drv_init          =
        dev->externflash_drv_init;
    _externflash_drv_dev[index].externflash_drv_deinit        =
        dev->externflash_drv_deinit;
    _externflash_drv_dev[index].externflash_drv_read          =
        dev->externflash_drv_read;
    _externflash_drv_dev[index].externflash_drv_write         =
        dev->externflash_drv_write;
    _externflash_drv_dev[index].externflash_drv_write_noerase =
        dev->externflash_drv_write_noerase;
    _externflash_drv_dev[index].externflash_drv_erasechip     =
        dev->externflash_drv_erasechip;
    _externflash_drv_dev[index].externflash_drv_erasesector   =
        dev->externflash_drv_erasesector;
    _externflash_drv_dev[index].externflash_drv_wakeup        =
        dev->externflash_drv_wakeup;
    _externflash_drv_dev[index].externflash_drv_sleep         =
        dev->externflash_drv_sleep;
    _externflash_drv_dev[index].externflash_drv_test          =
        dev->externflash_drv_test;

    /* 最近注册的驱动成为当前生效驱动 */
    _s_current_externflash_dev_idx = index;

    return true;
}

/**
 * @brief 调当前驱动的 init 回调。
 */
void bsp_wrapper_flash_init(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();

    /* 回调不存在则静默跳过, 避免空指针 */
    if (NULL != dev->externflash_drv_init)
    {
        dev->externflash_drv_init(dev);
    }
}

/**
 * @brief 调当前驱动的 deinit 回调。
 */
void bsp_wrapper_flash_deinit(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();

    if (NULL != dev->externflash_drv_deinit)
    {
        dev->externflash_drv_deinit(dev);
    }
}

/**
 * @brief 调当前驱动的 read 回调。
 *
 * @param[in]  addr  : 读起始地址。
 * @param[in]  size  : 读取长度。
 * @param[out] p_buf : 读缓冲。
 */
void bsp_wrapper_flash_read(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    externflash_drv_t *dev = get_current_externflash_drv();

    if (NULL != dev->externflash_drv_read)
    {
        dev->externflash_drv_read(dev, addr, size, p_buf);
    }
}

/**
 * @brief 调当前驱动的 write 回调（先擦后写）。
 */
void bsp_wrapper_flash_write(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    externflash_drv_t *dev = get_current_externflash_drv();

    if (NULL != dev->externflash_drv_write)
    {
        dev->externflash_drv_write(dev, addr, size, p_buf);
    }
}

/**
 * @brief 调当前驱动的 write 回调（不擦除）。
 */
void bsp_wrapper_flash_write_noerase(uint32_t addr, uint32_t size,
                                     uint8_t *p_buf)
{
    externflash_drv_t *dev = get_current_externflash_drv();

    if (NULL != dev->externflash_drv_write_noerase)
    {
        dev->externflash_drv_write_noerase(dev, addr, size, p_buf);
    }
}

/**
 * @brief 调当前驱动的整片擦除。
 */
void bsp_wrapper_flash_erasechip(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();

    if (NULL != dev->externflash_drv_erasechip)
    {
        dev->externflash_drv_erasechip(dev);
    }
}

/**
 * @brief 调当前驱动的扇区擦除。
 *
 * @param[in] addr : 扇区起始地址。
 */
void bsp_wrapper_flash_erasesector(uint32_t addr)
{
    externflash_drv_t *dev = get_current_externflash_drv();

    if (NULL != dev->externflash_drv_erasesector)
    {
        dev->externflash_drv_erasesector(dev, addr);
    }
}

/**
 * @brief 调当前驱动的唤醒。
 */
void bsp_wrapper_flash_wakeup(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();

    if (NULL != dev->externflash_drv_wakeup)
    {
        dev->externflash_drv_wakeup(dev);
    }
}

/**
 * @brief 调当前驱动的睡眠。
 */
void bsp_wrapper_flash_sleep(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();

    if (NULL != dev->externflash_drv_sleep)
    {
        dev->externflash_drv_sleep(dev);
    }
}

/**
 * @brief 调当前驱动的自检查。
 */
void bsp_wrapper_flash_test(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();

    if (NULL != dev->externflash_drv_test)
    {
        dev->externflash_drv_test(dev);
    }
}
