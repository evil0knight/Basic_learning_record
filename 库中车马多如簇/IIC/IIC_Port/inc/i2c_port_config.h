#ifndef I2C_PORT_CONFIG_H
#define I2C_PORT_CONFIG_H

/*
 * Include the target i2c.h here when hardware I2C handles are used.
 * 使用硬件I2C句柄时，在此处包含目标工程的i2c.h。
 */

/* I2C logical slot table. Replace entries with board resources. */
/* I2C逻辑槽位表。移植时用目标板资源替换表项。 */
static st_core_i2c_slot_t g_i2c_slots[CORE_I2C_PORT_MAX] = {0};

#endif
