# USART Port

[← UART 模块](../MOC.md) | [← 主页](../../../index.md)

USART 平台抽象层（platform_mcu 层）：`core_usart_*` 统一接口，内部转 HAL，支持阻塞收发、DMA 收发、DMA + 空闲中断、回调注册。裸机 / OS 通用（无 OS 依赖）。

## 文件

| 文件 | 作用 |
| --- | --- |
| [usart_port.h](./inc/usart_port.h) / [usart_port.c](./src/usart_port.c) | `core_usart_*` 接口 + HAL 分发 + DMA + 回调桥 |
| [usart_port_config.h](./inc/usart_port_config.h) | 槽位表（HAL 句柄 + 波特率 + 超时） |

## 移植

1. 复制 `UART_Port/inc` 和 `UART_Port/src` 内全部文件到目标工程 `02_Platform/platform_mcu/usart/`。
2. 添加 `UART_Port/inc`、`UART_Port/src` 头文件搜索路径。
3. `usart_port.c` 加入编译。
4. 确认 CubeMX 生成 `usart.h` / `usart.c`（含 `MX_USART1_UART_Init`），并勾选 USART1 的 DMA 收发（用 DMA 功能时）。
5. 在 [usart_port_config.h](./inc/usart_port_config.h) 填 HAL 句柄、波特率、默认超时。

## 调用接口

### 阻塞收发

```c
#include "usart_port.h"

uint8_t rx[16];

core_usart_init();
core_usart_transmit(CORE_USART1, (uint8_t *)"hi\r\n", 4U, 1000U);
core_usart_receive(CORE_USART1, rx, sizeof(rx), 1000U);
```

### DMA + 空闲中断接收（一帧触发回调）

```c
#include "usart_port.h"

static uint8_t s_rx[128];

static void on_rx(uint8_t *data, uint16_t size)
{
    /* data = s_rx，size = 本次收到字节数；在这里解析帧 */
    (void)data;
    (void)size;
}

void uart_setup(void)
{
    core_usart_init();
    core_usart_register_rx_callback(CORE_USART1, on_rx);
    core_usart_receive_to_idle_dma(CORE_USART1, s_rx, sizeof(s_rx));
}
```

一帧结束（总线空闲）触发 `on_rx`，`size` 是本帧长度，之后需再次调 `core_usart_receive_to_idle_dma` 重启接收。

### DMA 发送

```c
core_usart_register_tx_callback(CORE_USART1, on_tx_done);
core_usart_transmit_dma(CORE_USART1, buf, len);
```

## OTA 用法

OTA 收下载命令 `0x11 0x22 0x33` 用空闲中断：

```c
static volatile uint8_t s_ota_cmd_ready = 0U;
static uint8_t s_ota_cmd[4];

static void ota_rx_callback(uint8_t *data, uint16_t size)
{
    if ((size >= 3U) &&
        (data[0] == 0x11U) && (data[1] == 0x22U) && (data[2] == 0x33U))
    {
        s_ota_cmd_ready = 1U;
    }
}

void ota_uart_start(void)
{
    core_usart_register_rx_callback(CORE_USART1, ota_rx_callback);
    core_usart_receive_to_idle_dma(CORE_USART1, s_ota_cmd, sizeof(s_ota_cmd));
}
```

`ota_adapter_wait_download_request()` 里轮询 `s_ota_cmd_ready` 即可。

## 依赖

- HAL UART Driver + DMA（CubeMX 生成）
- 无 OS 依赖（裸机 / OS 通用）
