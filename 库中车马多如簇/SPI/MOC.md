# SPI

[← 模块总览](../MOC.md)|[← 主页](../../index.md)

[←uart](../UART内含串口助手安装包/MOC.md)|

---

## [spi.h](./spi.h),[spi.c](./spi.c)

## 移植

1. 修改 [spi.h](./spi.h)（第 28-29 行）的 `F_CS_Pin`、`F_CS_GPIO_Port`，并同步修改 [spi.c](./spi.c)（第 40、43 行）的片选 GPIO。
2. 按实际硬件修改 [spi.c](./spi.c)（第 52-53 行）的 SPI/GPIO 时钟，（第 58、61、63-65 行）的 SCK、MISO、MOSI 引脚与复用映射。
3. 按设备时序修改 [spi.c](./spi.c)（第 72-74 行）的分频、`SPI_CPHA`、`SPI_CPOL`。
4. 使用 `SPI1_WriteByte()`、`SPI1_ReadByte()` 前，工程需在 [spi.c](./spi.c)（第 122、155 行）提供 `millis()` 毫秒计时函数。
5. Keil 将 `spi.c`、`spi.h` 添加到工程，并将 SPI 外设库源文件加入编译。
6. 要使用 SPI 的文件添加 `#include "spi.h"`，初始化时调用 `SPI1_Init()`。

## SPI简单介绍:

1. SCK(时钟)+MOSI(主出从入)+MISO(主入从出)+CS(从机片选线,拉低相互对话)
2. 可达几十MHz
3. CS片选，每增一设备多一根线
4. 全双工
5. 协议简单

## 拓展:

#### 对于线和速度的取舍而扩展:

1. 3-wire SPI:SCK+CS+SDA
2. 4-wire SPI:见上面*SPI简单介绍*↑
3. DSPI:半双工,MOSI和MISO不再分方向
4. QSPI:半双工,再复用可能存在的/WP(写保护),/HOLD(通信暂停),这两个是在少数器件上会有,废物利用

#### CS线分为硬件和软件管理:

* **硬件管理** ：SPI控制器自动控制CS信号，传输开始自动拉低，结束拉高
  * 优点：时序精确，不占CPU
  * 缺点：灵活性差，多字节传输中CS可能意外拉高
* **软件管理** ：用 GPIO 手动控制CS电平
  * 优点：完全可控，适合多从机、不规则传输
  * 缺点：需要手动保证时序

实际项目  **90%用软件管理CS** ，因为很多SPI设备要求CS在整个事务期间保持低电平。
