#include "bsp_xxx_driver.h"
#include <stdlib.h>
#include <string.h>

/*
 * xxx 驱动实现（寄存器操作）
 * 总线逐字节接口已注入，这里只按命令码拼字节流。
 */

/* 私有数据 */
struct xxx_private_data
{
    uint8_t is_sleep;
};

/* 实例化：挂载接口 + 分配私有数据 + 挂载 pf_* 函数 */
static xxx_status_t xxx_inst_impl(void *const p_xxx_instance,
                                   xxx_bus_interface_t *const p_bus_instance,
                                   xxx_timebase_interface_t *const p_timebase_instance,
                                   xxx_os_delay_interface_t *const p_os_delay_instance)
{
    xxx_driver_t *p_driver = (xxx_driver_t *)p_xxx_instance;
    xxx_private_data_t *p_private_data;

    if ((p_driver == NULL) || (p_bus_instance == NULL) ||
        (p_timebase_instance == NULL) || (p_os_delay_instance == NULL))
    {
        return XXX_ERROR_PARAMETER;
    }

    p_private_data = (xxx_private_data_t *)malloc(sizeof(xxx_private_data_t));
    if (p_private_data == NULL)
    {
        return XXX_ERROR_NOMEM;
    }
    memset(p_private_data, 0, sizeof(xxx_private_data_t));

    p_driver->p_bus_instance = p_bus_instance;
    p_driver->p_timebase_instance = p_timebase_instance;
    p_driver->p_os_delay_instance = p_os_delay_instance;
    p_driver->p_private_data = p_private_data;

    /* 挂载实例函数 */
    p_driver->pf_inst = xxx_inst_impl;
    p_driver->pf_read = xxx_read;
    p_driver->pf_write = xxx_write;

    return XXX_OK;
}

/* 实例化入口：先挂载 pf_inst 再调用 */
xxx_status_t xxx_inst(xxx_driver_t *const p_xxx_instance,
                      xxx_bus_interface_t *const p_bus_instance,
                      xxx_timebase_interface_t *const p_timebase_instance,
                      xxx_os_delay_interface_t *const p_os_delay_instance)
{
    if (p_xxx_instance == NULL)
    {
        return XXX_ERROR_PARAMETER;
    }
    p_xxx_instance->pf_inst = xxx_inst_impl;
    return p_xxx_instance->pf_inst(p_xxx_instance, p_bus_instance,
                                    p_timebase_instance, p_os_delay_instance);
}

/* 读数据：按器件命令码实现 */
xxx_status_t xxx_read(void *const p_xxx_instance, uint8_t *p_data,
                      uint32_t addr, uint32_t size)
{
    xxx_driver_t *p_driver = (xxx_driver_t *)p_xxx_instance;

    if (p_driver == NULL)
    {
        return XXX_ERROR_PARAMETER;
    }

    (void)p_data;
    (void)addr;
    (void)size;
    /*
     * 示例：
     *   p_driver->p_bus_instance->pf_bus_cs_set();
     *   p_driver->p_bus_instance->pf_bus_write_byte(XXX_CMD_READ);
     *   ... 写地址字节 ...
     *   循环 p_driver->p_bus_instance->pf_bus_read_byte() 读回
     *   p_driver->p_bus_instance->pf_bus_cs_deset();
     */
    return XXX_OK;
}

/* 写数据：按器件命令码实现 */
xxx_status_t xxx_write(void *const p_xxx_instance, const uint8_t *p_data,
                       uint32_t addr, uint32_t size)
{
    xxx_driver_t *p_driver = (xxx_driver_t *)p_xxx_instance;

    if (p_driver == NULL)
    {
        return XXX_ERROR_PARAMETER;
    }

    (void)p_data;
    (void)addr;
    (void)size;
    /*
     * 示例：
     *   p_driver->p_bus_instance->pf_bus_cs_set();
     *   p_driver->p_bus_instance->pf_bus_write_byte(XXX_CMD_WRITE);
     *   ... 写地址字节 ...
     *   循环 p_driver->p_bus_instance->pf_bus_write_byte(p_data[i])
     *   p_driver->p_bus_instance->pf_bus_cs_deset();
     */
    return XXX_OK;
}

/* 读器件 ID（按需） */
xxx_status_t read_xxx_id(xxx_driver_t *const p_xxx_instance,
                         uint16_t *const p_id)
{
    (void)p_xxx_instance;
    (void)p_id;
    return XXX_OK;
}
