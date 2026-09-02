#ifndef BSP_ADAPTER_PORT_FLASH_H
#define BSP_ADAPTER_PORT_FLASH_H

#include <stdint.h>

/*
 * 外部 Flash Adapter（OS / 裸机二选一，用 FLASH_BARE_METAL 切换）
 *
 * OS 版（未定义 FLASH_BARE_METAL）：
 *   在目标工程 app_periph_register() 里调 bsp_adapter_flash_register()，绑定接口 +
 *   发事件 + 注册函数表；同时定义 flash_handler_all_input_arg 供 handler 任务用。
 *
 * 裸机版（定义 FLASH_BARE_METAL）：
 *   不用 handler / wrapper / service，直接同步调 Driver，对外留
 *   flash_bare_init / flash_bare_read / flash_bare_write / flash_bare_erase_all。
 */

/* Flash 使用的 SPI 逻辑 port（在目标工程 spi_port_config.h 静态表里配置） */
#ifndef FLASH_SPI_PORT
#define FLASH_SPI_PORT 0U
#endif

/* 单次 SPI 传输超时（毫秒） */
#ifndef FLASH_SPI_TIMEOUT_MS
#define FLASH_SPI_TIMEOUT_MS 100U
#endif

#ifdef FLASH_BARE_METAL
/* 裸机同步接口 */
int flash_bare_init(void);
int flash_bare_read(uint32_t addr, uint8_t *buf, uint32_t size);
int flash_bare_write(uint32_t addr, const uint8_t *buf, uint32_t size);
int flash_bare_erase_all(void);
#else
/* OS 注册入口 */
void bsp_adapter_flash_register(void);
#endif

#endif /* BSP_ADAPTER_PORT_FLASH_H */
