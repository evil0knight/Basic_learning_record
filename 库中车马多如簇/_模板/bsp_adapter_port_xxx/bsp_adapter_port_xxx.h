#ifndef BSP_ADAPTER_PORT_XXX_H
#define BSP_ADAPTER_PORT_XXX_H

#include <stdint.h>

/*
 * xxx Adapter（OS / 裸机二选一，用 XXX_BARE_METAL 切换）
 *
 * OS 版（未定义 XXX_BARE_METAL）：
 *   在 app_periph_register() 里调 bsp_adapter_xxx_register()，绑定接口 +
 *   发事件 + 注册函数表；同时定义 xxx_handler_all_input_arg 供 handler 任务用。
 *
 * 裸机版（定义 XXX_BARE_METAL）：
 *   不用 handler / wrapper / OS 队列，直接同步调 Driver，对外留
 *   xxx_bare_init / xxx_bare_read / xxx_bare_write。
 */

/* xxx 使用的总线槽位号（在目标工程总线配置表静态表里配置） */
#ifndef XXX_BUS_PORT
#define XXX_BUS_PORT 0U
#endif

/* 单次总线传输超时 */
#ifndef XXX_TIMEOUT_MS
#define XXX_TIMEOUT_MS 100U
#endif

#ifdef XXX_BARE_METAL
/* 裸机同步接口 */
int xxx_bare_init(void);
int xxx_bare_read(uint32_t addr, uint8_t *buf, uint32_t size);
int xxx_bare_write(uint32_t addr, const uint8_t *buf, uint32_t size);
#else
/* OS 注册入口 */
void bsp_adapter_xxx_register(void);
#endif

#endif /* BSP_ADAPTER_PORT_XXX_H */
