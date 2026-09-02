# Ymodem 传输协议

[← 返回 OTA](./MOC.md) | [← 主页](../../index.md)

Ymodem 负责可靠传输文件；UART 初始化、Flash 擦写和升级状态由对应库及 OTA 状态机负责。

## 协议要点

- 起始包携带文件名和长度，数据包使用 128 字节或 1K 负载。
- 接收端校验包序号和 CRC16，正确后回复 `ACK`，错误回复 `NAK`，发送端以 `EOT` 结束。
- 接收流程为握手、文件头、数据包循环、结束确认；取消使用 `CAN`。

## 可复制实现

- [Ymodem MOC](../../库中车马多如簇/Ymodem/MOC.md)
- [ymodem.c](../../库中车马多如簇/Ymodem/ymodem.c) / [ymodem.h](../../库中车马多如簇/Ymodem/ymodem.h)
- [common.c](../../库中车马多如簇/Ymodem/common.c) / [common.h](../../库中车马多如簇/Ymodem/common.h)

先按[Ymodem 库配置](../../库中车马多如簇/Ymodem/MOC.md)完成 UART、Flash Wrapper 和 `ymodem_config.h`，再在 BootLoader 或 OTA Service 中调用：

```c
#include "ymodem.h"

static uint32_t ymodem_buffer[256];
int32_t received_size = Ymodem_Receive((uint8_t *)ymodem_buffer);
```

`ymodem_buffer` 至少为 `PACKET_1K_SIZE` 字节。模式 1 将 `YMODEM_FLASH_START_ADDRESS` 配置为 `OTA_APP_ADDRESS`；模式 2/3 配置为 `OTA_AREA_A_ADDRESS`，可写范围由 OTA 逻辑区大小限制。

