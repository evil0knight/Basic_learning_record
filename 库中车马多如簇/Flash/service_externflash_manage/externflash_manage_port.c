#include "externflash_manage_port.h"
#include "bsp_wrapper_flash.h"

/*
 * 外部 Flash 平台 port 实现：转发到 bsp_wrapper_flash（函数表注册版）。
 */

void externflash_init(void)
{
    bsp_wrapper_flash_init();
}

void externflash_deinit(void)
{
    bsp_wrapper_flash_deinit();
}

void externflash_read(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    bsp_wrapper_flash_read(addr, size, p_buf);
}

void externflash_write(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    bsp_wrapper_flash_write(addr, size, p_buf);
}

void externflash_write_noerase(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    bsp_wrapper_flash_write_noerase(addr, size, p_buf);
}

void externflash_erasechip(void)
{
    bsp_wrapper_flash_erasechip();
}

void externflash_erasesector(uint32_t addr)
{
    bsp_wrapper_flash_erasesector(addr);
}

void externflash_wakeup(void)
{
    bsp_wrapper_flash_wakeup();
}

void externflash_sleep(void)
{
    bsp_wrapper_flash_sleep();
}

void externflash_test(void)
{
    bsp_wrapper_flash_test();
}
