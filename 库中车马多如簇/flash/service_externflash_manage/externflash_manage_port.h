#ifndef EXTERNFLASH_MANAGE_PORT_H
#define EXTERNFLASH_MANAGE_PORT_H

#include <stdint.h>

/*
 * 外部 Flash 平台 port：把分区服务的读写请求转发到 bsp_wrapper_flash。
 * 本层不包含器件 / OS 类型，只是接口转发。
 */

void externflash_init(void);
void externflash_deinit(void);
void externflash_read(uint32_t addr, uint32_t size, uint8_t *p_buf);
void externflash_write(uint32_t addr, uint32_t size, uint8_t *p_buf);
void externflash_write_noerase(uint32_t addr, uint32_t size, uint8_t *p_buf);
void externflash_erasesector(uint32_t addr);
void externflash_wakeup(void);
void externflash_sleep(void);
void externflash_test(void);
void externflash_erasechip(void);

#endif /* EXTERNFLASH_MANAGE_PORT_H */
