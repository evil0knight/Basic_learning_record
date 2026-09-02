#include "bsp_wrapper_xxx.h"

/*
 * xxx Wrapper 实现：保存注册的函数表，read/write 等转发到注册的驱动。
 * 本层无状态、无 OS 依赖，纯函数表转发。
 */

#define XXX_DRV_MAX_NUM (2U)

static xxx_drv_t _xxx_drv_dev[XXX_DRV_MAX_NUM];
static uint32_t _s_current_xxx_dev_idx = 0U;

static xxx_drv_t *get_current_xxx_drv(void)
{
    return &_xxx_drv_dev[_s_current_xxx_dev_idx];
}

bool bsp_wrapper_xxx_reg(uint32_t index, xxx_drv_t *dev)
{
    if ((index >= XXX_DRV_MAX_NUM) || (dev == NULL))
    {
        return false;
    }

    _xxx_drv_dev[index].idx = index;
    _xxx_drv_dev[index].dev_id = dev->dev_id;
    _xxx_drv_dev[index].user_data = dev->user_data;
    _xxx_drv_dev[index].drv_init = dev->drv_init;
    _xxx_drv_dev[index].drv_deinit = dev->drv_deinit;
    _xxx_drv_dev[index].drv_read = dev->drv_read;
    _xxx_drv_dev[index].drv_write = dev->drv_write;

    _s_current_xxx_dev_idx = index;

    return true;
}

void bsp_wrapper_xxx_init(void)
{
    xxx_drv_t *dev = get_current_xxx_drv();
    if (dev->drv_init != NULL)
    {
        dev->drv_init(dev);
    }
}

void bsp_wrapper_xxx_deinit(void)
{
    xxx_drv_t *dev = get_current_xxx_drv();
    if (dev->drv_deinit != NULL)
    {
        dev->drv_deinit(dev);
    }
}

void bsp_wrapper_xxx_read(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    xxx_drv_t *dev = get_current_xxx_drv();
    if (dev->drv_read != NULL)
    {
        dev->drv_read(dev, addr, size, p_buf);
    }
}

void bsp_wrapper_xxx_write(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    xxx_drv_t *dev = get_current_xxx_drv();
    if (dev->drv_write != NULL)
    {
        dev->drv_write(dev, addr, size, p_buf);
    }
}
