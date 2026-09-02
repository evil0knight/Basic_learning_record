# 裸机使用 Flash Driver（移植教程）

[← Flash 模块总览](./MOC.md)

> 裸机（无 RTOS）不用 Handler / Service，直接同步调 Driver。定义 `FLASH_BARE_METAL` 后，[bsp_adapter_port_flash](./bsp_adapter_port_flash/bsp_adapter_port_flash.h) 对外只留 4 个函数。

## 需要放置的文件

| 文件 | 放到目标工程 |
| --- | --- |
| `bsp_adapter_port_flash.h` / `.c` | `02_Platform/platform_bsp/flash/` |
| `bsp_w25q64_driver.h` / `.c` + `bsp_w25q64_reg.h` | `03_Bsp/bsp_storage/driver/W25Q64/` |
| `spi_port.h` / `spi_port.c` + `spi_port_config.h`（软件 SPI 再加 `soft_spi.h` / `soft_spi.c`） | `02_Platform/platform_mcu/spi/` |
| `gpio_port.h` / `gpio_port.c` | `02_Platform/platform_mcu/gpio/` |

> 文件来源：Driver / Reg 在 [W25Q](./W25Q/MOC.md)，SPI Port 在 [SPI Port](../SPI/SPI_Port/MOC.md)，GPIO Port 在 [GPIO Port](../代码架构/firmware/04_platform/platform_mcu/gpio/MOC.md)。

## 怎么用

1. 目标工程全局宏加 `FLASH_BARE_METAL`（Keil 在 `C/C++ -> Define`，或工程头文件 `#define FLASH_BARE_METAL`）。
2. 调用：

```c
#include "bsp_adapter_port_flash.h"

uint8_t buf[256];

if (flash_bare_init() != 0)
{
    /* 失败：缺接口 / 读不到 ID / 无 heap */
}

flash_bare_read(0x000000, buf, sizeof(buf));    /* 同步读 */
flash_bare_write(0x000000, buf, sizeof(buf));   /* 同步写（先擦后写） */
flash_bare_erase_all();                          /* 全片擦除 */
```

## 移植要改什么

### 1. `bsp_adapter_port_flash.h`

| 宏 | 改什么 |
| --- | --- |
| `FLASH_BARE_METAL` | 打开裸机模式，adapter 只留同步接口，不依赖 OS |
| `FLASH_SPI_PORT` | Flash 用的 SPI 槽位号，改成 `spi_port_config.h` 里为 Flash 配的 `g_spi_slots[]` 下标 |
| `FLASH_SPI_TIMEOUT_MS` | 单次 SPI 传输超时（毫秒），默认 100，按需调整 |

### 2. `bsp_adapter_port_flash.c` 的临界区

`flash_critical_enter` / `flash_critical_exit` 在裸机下默认是空函数（单线程够用）。若中断里也会访问 SPI，改成关中断：

```c
static void flash_critical_enter(void) { __disable_irq(); }
static void flash_critical_exit(void)  { __enable_irq(); }
```

### 3. `spi_port_config.h` 的 SPI 槽位表

`FLASH_SPI_PORT` 指向的槽位要在 `g_spi_slots[]` 里配好（硬件 / 软件 SPI、引脚、lock / unlock），配置方法见 [SPI Port](../SPI/SPI_Port/MOC.md)。裸机下硬件 SPI 的 lock / unlock 填 NULL，软件 SPI 填关中断。
