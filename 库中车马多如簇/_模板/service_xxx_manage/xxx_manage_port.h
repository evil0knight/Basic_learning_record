#ifndef XXX_MANAGE_PORT_H
#define XXX_MANAGE_PORT_H

#include <stdint.h>

/*
 * xxx 平台 port：把分区服务的读写请求转发到 bsp_wrapper_xxx。
 * 本层不包含器件 / OS 类型，只是接口转发。
 */

void xxx_port_init(void);
void xxx_port_deinit(void);
void xxx_port_read(uint32_t addr, uint32_t size, uint8_t *p_buf);
void xxx_port_write(uint32_t addr, uint32_t size, uint8_t *p_buf);
/* 按需增加：xxx_port_erasesector / xxx_port_wakeup / ... */

#endif /* XXX_MANAGE_PORT_H */
