#ifndef BSP_ADAPTER_PORT_EEPROM_H
#define BSP_ADAPTER_PORT_EEPROM_H

#include <stdint.h>

/*
 * EEPROM Adapter 注册入口
 *
 * 在目标工程的 app_periph_register() 里调用 bsp_adapter_eeprom_register()，完成：
 *   1. 绑定 I2C Port（core_i2c 逐字节接口）；
 *   2. 实现 eeprom_drv_t 函数表（内部直接同步调用 AT24C02 Driver）；
 *   3. 注册函数表到 wrapper。
 *
 * EEPROM 属 A 类（同步）外设，不涉及事件队列 / 任务 / 信号量。
 */

/* EEPROM 使用的 I2C 逻辑槽位（在目标工程 i2c_port_config.h 静态表里配置） */
#ifndef EEPROM_I2C_PORT
#define EEPROM_I2C_PORT 0U
#endif

/* AT24C02 的 A2:A0 地址脚电平（0~7） */
#ifndef EEPROM_DEVICE_ADDRESS_PINS
#define EEPROM_DEVICE_ADDRESS_PINS 0U
#endif

/* 单次 I2C 传输超时（毫秒） */
#ifndef EEPROM_I2C_TIMEOUT_MS
#define EEPROM_I2C_TIMEOUT_MS 10U
#endif

void bsp_adapter_eeprom_register(void);

#endif /* BSP_ADAPTER_PORT_EEPROM_H */
