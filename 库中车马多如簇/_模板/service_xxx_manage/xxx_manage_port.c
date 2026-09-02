#include "xxx_manage_port.h"
#include "bsp_wrapper_xxx.h"

/*
 * xxx 平台 port 实现：转发到 bsp_wrapper_xxx（函数表注册版）。
 */

void xxx_port_init(void)
{
    bsp_wrapper_xxx_init();
}

void xxx_port_deinit(void)
{
    bsp_wrapper_xxx_deinit();
}

void xxx_port_read(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    bsp_wrapper_xxx_read(addr, size, p_buf);
}

void xxx_port_write(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    bsp_wrapper_xxx_write(addr, size, p_buf);
}
