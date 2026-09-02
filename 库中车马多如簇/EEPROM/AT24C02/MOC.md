# AT24C02

[← EEPROM](../MOC.md) | [← 主页](../../../index.md)

AT24C02 EEPROM 器件层：寄存器 Driver（同步 A 类，无 Handler）。

| 文件 | 作用 |
| --- | --- |
| [bsp_at24c02_driver.h](./bsp_at24c02_driver.h) / [bsp_at24c02_driver.c](./bsp_at24c02_driver.c) | Driver：寄存器操作（读/写/擦除），I2C 逐字节接口注入 |
| [bsp_at24c02_reg.h](./bsp_at24c02_reg.h) | 器件几何 + 地址/轮询参数 |

Driver 的 I2C 逐字节接口（`at24c02_i2c_interface_t`）由 [bsp_adapter_port_eeprom](../bsp_adapter_port_eeprom/bsp_adapter_port_eeprom.c) 用 `core_i2c` 适配注入，Driver 本身不依赖具体 I2C 类型。

写操作按 8 字节页自动拆分，每页写后用 ACK 轮询等待内部写周期（约 5ms）结束；读为单次连续读。
