#include "bsp_wrapper_flash.h"

/*
 * 外部 Flash Wrapper 实现：保存注册的函数表，read/write 等转发到注册的驱动。
 * 本层无状态、无 OS 依赖，纯函数表转发。
 */

#define EXTERNFLASH_DRV_MAX_NUM (2U)

static externflash_drv_t _externflash_drv_dev[EXTERNFLASH_DRV_MAX_NUM];
static uint32_t _s_current_externflash_dev_idx = 0U;

static externflash_drv_t *get_current_externflash_drv(void)
{
    return &_externflash_drv_dev[_s_current_externflash_dev_idx];
}

bool bsp_wrapper_flash_reg(uint32_t index, externflash_drv_t *dev)
{
    if ((index >= EXTERNFLASH_DRV_MAX_NUM) || (dev == NULL))
    {
        return false;
    }

    _externflash_drv_dev[index].idx = index;
    _externflash_drv_dev[index].dev_id = dev->dev_id;
    _externflash_drv_dev[index].user_data = dev->user_data;
    _externflash_drv_dev[index].externflash_drv_init = dev->externflash_drv_init;
    _externflash_drv_dev[index].externflash_drv_deinit = dev->externflash_drv_deinit;
    _externflash_drv_dev[index].externflash_drv_read = dev->externflash_drv_read;
    _externflash_drv_dev[index].externflash_drv_write = dev->externflash_drv_write;
    _externflash_drv_dev[index].externflash_drv_write_noerase = dev->externflash_drv_write_noerase;
    _externflash_drv_dev[index].externflash_drv_erasechip = dev->externflash_drv_erasechip;
    _externflash_drv_dev[index].externflash_drv_erasesector = dev->externflash_drv_erasesector;
    _externflash_drv_dev[index].externflash_drv_wakeup = dev->externflash_drv_wakeup;
    _externflash_drv_dev[index].externflash_drv_sleep = dev->externflash_drv_sleep;
    _externflash_drv_dev[index].externflash_drv_test = dev->externflash_drv_test;

    _s_current_externflash_dev_idx = index;

    return true;
}

void bsp_wrapper_flash_init(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();
    if (dev->externflash_drv_init != NULL)
    {
        dev->externflash_drv_init(dev);
    }
}

void bsp_wrapper_flash_deinit(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();
    if (dev->externflash_drv_deinit != NULL)
    {
        dev->externflash_drv_deinit(dev);
    }
}

void bsp_wrapper_flash_read(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    externflash_drv_t *dev = get_current_externflash_drv();
    if (dev->externflash_drv_read != NULL)
    {
        dev->externflash_drv_read(dev, addr, size, p_buf);
    }
}

void bsp_wrapper_flash_write(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    externflash_drv_t *dev = get_current_externflash_drv();
    if (dev->externflash_drv_write != NULL)
    {
        dev->externflash_drv_write(dev, addr, size, p_buf);
    }
}

void bsp_wrapper_flash_write_noerase(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    externflash_drv_t *dev = get_current_externflash_drv();
    if (dev->externflash_drv_write_noerase != NULL)
    {
        dev->externflash_drv_write_noerase(dev, addr, size, p_buf);
    }
}

void bsp_wrapper_flash_erasechip(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();
    if (dev->externflash_drv_erasechip != NULL)
    {
        dev->externflash_drv_erasechip(dev);
    }
}

void bsp_wrapper_flash_erasesector(uint32_t addr)
{
    externflash_drv_t *dev = get_current_externflash_drv();
    if (dev->externflash_drv_erasesector != NULL)
    {
        dev->externflash_drv_erasesector(dev, addr);
    }
}

void bsp_wrapper_flash_wakeup(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();
    if (dev->externflash_drv_wakeup != NULL)
    {
        dev->externflash_drv_wakeup(dev);
    }
}

void bsp_wrapper_flash_sleep(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();
    if (dev->externflash_drv_sleep != NULL)
    {
        dev->externflash_drv_sleep(dev);
    }
}

void bsp_wrapper_flash_test(void)
{
    externflash_drv_t *dev = get_current_externflash_drv();
    if (dev->externflash_drv_test != NULL)
    {
        dev->externflash_drv_test(dev);
    }
}
