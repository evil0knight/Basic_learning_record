#include "bsp_wrapper_eeprom.h"

/*
 * EEPROM Wrapper 实现：保存注册的函数表，init/read/write 等转发到注册的驱动。
 * 本层无状态、无 OS 依赖，纯函数表转发。
 */

#define EEPROM_DRV_MAX_NUM (2U)

static eeprom_drv_t _eeprom_drv_dev[EEPROM_DRV_MAX_NUM];
static uint32_t _s_current_eeprom_dev_idx = 0U;

static eeprom_drv_t *get_current_eeprom_drv(void)
{
    return &_eeprom_drv_dev[_s_current_eeprom_dev_idx];
}

bool bsp_wrapper_eeprom_reg(uint32_t index, eeprom_drv_t *dev)
{
    if ((index >= EEPROM_DRV_MAX_NUM) || (dev == NULL))
    {
        return false;
    }

    _eeprom_drv_dev[index].idx = index;
    _eeprom_drv_dev[index].dev_id = dev->dev_id;
    _eeprom_drv_dev[index].user_data = dev->user_data;
    _eeprom_drv_dev[index].eeprom_drv_init = dev->eeprom_drv_init;
    _eeprom_drv_dev[index].eeprom_drv_deinit = dev->eeprom_drv_deinit;
    _eeprom_drv_dev[index].eeprom_drv_read = dev->eeprom_drv_read;
    _eeprom_drv_dev[index].eeprom_drv_write = dev->eeprom_drv_write;
    _eeprom_drv_dev[index].eeprom_drv_erase = dev->eeprom_drv_erase;

    _s_current_eeprom_dev_idx = index;

    return true;
}

eeprom_status_t bsp_wrapper_eeprom_init(void)
{
    eeprom_drv_t *dev = get_current_eeprom_drv();
    if (dev->eeprom_drv_init != NULL)
    {
        return dev->eeprom_drv_init(dev);
    }
    return EEPROM_ERROR_NOT_READY;
}

eeprom_status_t bsp_wrapper_eeprom_deinit(void)
{
    eeprom_drv_t *dev = get_current_eeprom_drv();
    if (dev->eeprom_drv_deinit != NULL)
    {
        return dev->eeprom_drv_deinit(dev);
    }
    return EEPROM_ERROR_NOT_READY;
}

eeprom_status_t bsp_wrapper_eeprom_read(uint32_t addr, uint32_t size,
                                        uint8_t *p_buf)
{
    eeprom_drv_t *dev = get_current_eeprom_drv();
    if (dev->eeprom_drv_read != NULL)
    {
        return dev->eeprom_drv_read(dev, addr, size, p_buf);
    }
    return EEPROM_ERROR_NOT_READY;
}

eeprom_status_t bsp_wrapper_eeprom_write(uint32_t addr, uint32_t size,
                                         uint8_t *p_buf)
{
    eeprom_drv_t *dev = get_current_eeprom_drv();
    if (dev->eeprom_drv_write != NULL)
    {
        return dev->eeprom_drv_write(dev, addr, size, p_buf);
    }
    return EEPROM_ERROR_NOT_READY;
}

eeprom_status_t bsp_wrapper_eeprom_erase(void)
{
    eeprom_drv_t *dev = get_current_eeprom_drv();
    if (dev->eeprom_drv_erase != NULL)
    {
        return dev->eeprom_drv_erase(dev);
    }
    return EEPROM_ERROR_NOT_READY;
}
