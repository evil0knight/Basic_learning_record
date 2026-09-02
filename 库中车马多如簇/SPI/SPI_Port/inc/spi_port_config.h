#ifndef SPI_PORT_CONFIG_H
#define SPI_PORT_CONFIG_H

/*
 * Include the target spi.h here when hardware SPI handles are used.
 * 使用硬件SPI句柄时，在此处包含目标工程的spi.h。
 */

/* SPI logical slot table. Replace entries with board resources. */
/* SPI逻辑槽位表。移植时用目标板资源替换表项。 */
static st_core_spi_slot_t g_spi_slots[CORE_SPI_PORT_MAX] = {0};

#endif
