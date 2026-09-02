# EEPROM

[← 模块总览](../MOC.md) | [← 主页](../../index.md)

> [EEPROM知识](./知识/MOC.md)

AT24C02 EEPROM 驱动栈：同步 A 类（Wrapper + Adapter + Driver + Reg）。单次读写原子、快速，不需要后台线程，读写接口直接同步返回结果。

## 文件结构

| 目录 | 位置（目标工程） |
| --- | --- |
| [bsp_wrapper_eeprom](./bsp_wrapper_eeprom/bsp_wrapper_eeprom.h) | `02_Platform/platform_bsp/eeprom/` |
| [bsp_adapter_port_eeprom](./bsp_adapter_port_eeprom/bsp_adapter_port_eeprom.h) | `02_Platform/platform_bsp/eeprom/` |
| [AT24C02](./AT24C02/MOC.md) | `03_Bsp/bsp_storage/` |

## 接入目标工程

### 1. 配置宏

在 [bsp_adapter_port_eeprom.h](./bsp_adapter_port_eeprom/bsp_adapter_port_eeprom.h) 配置：

- `EEPROM_I2C_PORT`：EEPROM 用的 I2C 槽位号，即 `i2c_port_config.h` 的 `g_i2c_slots[]` 数组下标（0~7）
- `EEPROM_DEVICE_ADDRESS_PINS`：AT24C02 的 A2:A0 电平（0~7）
- `EEPROM_I2C_TIMEOUT_MS`：单次 I2C 传输超时（毫秒）

### 2. 注册流程

1. 先完成 [IIC Port](../IIC/MOC.md) 移植，`i2c_port_config.h` 里为 EEPROM 配置一个 I2C slot；
2. 在目标工程 `app_periph_register()` 里调用：

```c
#include "bsp_adapter_port_eeprom.h"

void app_periph_register(void)
{
    /* 其他外设注册... */
    bsp_adapter_eeprom_register();
}
```

> 裸机（无 RTOS）：EEPROM 是同步 A 类，adapter 直接同步调 Driver，天然支持裸机。在 `main()` 里直接调 `bsp_adapter_eeprom_register()`（代替 `app_periph_register()`），再调 `bsp_wrapper_eeprom_init/read/write`。I2C Port 的 lock 配置（软件 I2C 关中断、硬件 I2C 填 NULL）见 [IIC Port](../IIC/IIC_Port/MOC.md)。

3. 上层通过统一接口读写（`addr` 为器件内字节地址，AT24C02 范围 0~255）：

```c
#include "bsp_wrapper_eeprom.h"

uint8_t data[8];

if (bsp_wrapper_eeprom_init() == EEPROM_OK)
{
    (void)bsp_wrapper_eeprom_write(0U, sizeof(data), data);
    (void)bsp_wrapper_eeprom_read(0U, sizeof(data), data);
}
```

### 3. 增加第二片 / 新型号

- **增加第二片 AT24C02**：Adapter 当前只注册一个驱动（index 0）。多片时复制 Adapter 的驱动实例与函数表，用不同 `EEPROM_DEVICE_ADDRESS_PINS` 和 `bsp_wrapper_eeprom_reg(index, &drv)` 分别注册；上层按 `index` 选择。Driver / Reg 不修改。
- **增加新型号**（如 AT24C04）：新增该型号的 Driver + Reg，Adapter 按型号挂载对应 `pf_*`；Wrapper 函数表成员按外设增删。

### 4. 可复用与解耦

| 组件 | 是否 eeprom 专用 |
| --- | --- |
| `at24c02_i2c_interface_t`（I2C 逐字节接口） | 通用，其他 I2C 器件可照此模式 |
| 函数表注册（`eeprom_drv_t` + `reg`） | 通用，其他 Wrapper 可照此模式 |
| AT24C02 寄存器操作 / 器件几何 | eeprom 专用 |

## 依赖

- I2C Port（core_i2c 逐字节接口）—— 见 [IIC](../IIC/MOC.md)
