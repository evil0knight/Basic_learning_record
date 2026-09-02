# SPI

[← 模块总览](../MOC.md) | [← 主页](../../index.md)

> [spi知识](./知识/MOC.md)

---

| 写法                        | 文件                                                             |
| --------------------------- | ---------------------------------------------------------------- |
| 标准库硬件SPI               | [spi.h](./spi.h)、[spi.c](./spi.c)                                     |
| 软件SPI                     | [software_spi.h](./software_spi.h)、[software_spi.c](./software_spi.c) |
| 通用硬件/软件SPI Port       | [SPI Port](./SPI_Port/MOC.md)                                       |
| HAL库的已经在cubemx里生成了 |                                                                  |

## 新工程接入顺序

1. 建 `firmware\04_platform\platform_mcu\spi\`
2. 在CubeMX中配置硬件SPI或普通GPIO软件SPI引脚。
3. 给CS、SCK、MOSI、MISO设置User Label，确认宏生成到 `main.h`。
4. 可复用器件Driver只调用[SPI Port](./SPI_Port/MOC.md)的 `core_spi_*()`，不直接调用标准库SPI或独立软件SPI示例。
5. `spi.h/.c`返回 `en_spi_stdlib_status_t`，`software_spi.h/.c`返回 `en_software_spi_status_t`；新模块不得使用裸 `0/1`表示SPI状态。

## 信号

| 信号 | 方向     | 作用                           |
| ---- | -------- | ------------------------------ |
| SCK  | 主机输出 | 串行时钟                       |
| MOSI | 主机输出 | 主机发送数据                   |
| MISO | 主机输入 | 主机接收数据                   |
| CS   | 主机输出 | 选择具体从设备，通常低电平有效 |
