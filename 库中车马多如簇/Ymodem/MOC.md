# Ymodem

[← 模块总览](../MOC.md) | [← 主页](../../index.md)

[← 返回文件传输协议](../../术中自有万钟粟/OTA/文件传输协议.md)

## 文件

- [ymodem.h](./ymodem.h)
- [ymodem.c](./ymodem.c)
- [common.h](./common.h)
- [common.c](./common.c)

## 输入输出

当前版本通过串口接收 Ymodem 文件，并直接擦写 STM32F1 内部 Flash。

```text
电脑发送 .bin → SerialKeyPressed → Ymodem_Receive → Flash
电脑接收应答 ← SerialPutChar     ← C / ACK / NAK / CA
```

| 接口                                 | 输入                                                      | 输出                                                           |
| ------------------------------------ | --------------------------------------------------------- | -------------------------------------------------------------- |
| `Ymodem_Receive(buf)`              | 至少 1024 字节、按 4 字节对齐的临时缓冲区；文件从串口读取 | 文件写入 `BackAppAddress` 起始的 Flash；返回文件长度或错误码 |
| `Ymodem_Transmit(buf, name, size)` | 待发送数据地址、文件名、字节数                            | 通过串口发送文件；返回 0 表示成功，非 0 表示失败               |

接收时 `buf` 只暂存当前数据包，不保存整个文件；文件名存入 `ymodem.c` 中的全局数组 `file_name`。接收函数阻塞执行，返回后才继续执行调用方代码。

| `Ymodem_Receive` 返回值 | 含义                            |
| ------------------------- | ------------------------------- |
| `> 0`                   | 接收完成，返回文件字节数        |
| `0`                     | 发送端取消或错误次数超限        |
| `-1`                    | 文件大小检查失败                |
| `-2`                    | Flash 写入后读回不一致          |
| `-3`                    | 收到用户取消字符 `A` 或 `a` |

## 使用方法

1. 将四个源文件加入工程，提供 STM32F1 标准外设库依赖；处理下文列出的原工程依赖。
2. 初始化串口，适配收发函数，确认 Flash 下载分区及容量。
3. 调用 `Ymodem_Receive`，随后在电脑串口工具中选择 **Ymodem 发送**，发送 `.bin` 文件；两端串口参数必须一致。
4. 根据返回值处理结果。接收完成仅表示下载到 Flash，不会自动搬运固件或跳转 APP。

以下为接收调用片段，前提是串口、Flash 分区和工程依赖已配置完成：

```c
#include "common.h"

/* 使用 uint32_t 数组，保证 Flash 按字读取时的对齐。 */
static uint32_t rx_buffer[PACKET_1K_SIZE / sizeof(uint32_t)];

int32_t receive_firmware(void)
{
    int32_t size;

    FLASH_Unlock();
    size = Ymodem_Receive((uint8_t *)rx_buffer);
    FLASH_Lock();

    return size;
}
```

## 移植重点

| 文件 / 函数                                             | 需要适配的内容                                                                                          |
| ------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| [common.c](./common.c) · `SerialKeyPressed(key)`      | 这里判断需不需要开始协议                                                                                |
| [common.c](./common.c) · `SerialPutChar(c)`           | 发送一个字节。当前使用 USART1，并通过 PB9 控制 RS485 收发方向；普通 UART 应去掉方向控制                 |
| [common.h](./common.h) · `BackAppAddress`             | 文件下载地址，当前为 `0x0803D000`；`ApplicationAddress` 为 `0x08003000`，本接收函数不向该地址写入 |
| [common.h](./common.h) · `PAGE_SIZE` / `FLASH_SIZE` | 根据芯片实际页大小、容量和分区调整                                                                      |
| [ymodem.c](./ymodem.c) · `Ymodem_Receive`             | 换芯片或存储介质时，替换 `FLASH_ErasePage`、`FLASH_ProgramWord` 及直接读地址校验的操作              |
| [ymodem.c](./ymodem.c) · `Receive_Byte`               | 当前超时是循环次数，不是毫秒；更换主频或运行环境后应适配                                                |

## 当前依赖与待修正处

- `common.c` 依赖原工程的 `BspTime.h`，用于 `GetKey()` 的超时；`Main_Menu()` 还调用原工程 `download.c` 中的 `SerialDownload()`。这四个文件尚未完全独立，需补齐依赖或剥离不用的菜单功能。
- 接收路径读取了 CRC 字节，但没有实际校验 CRC；存在 CRC 计算函数不代表接收时已使用。
- 文件大小检查使用总 Flash 容量，没有按 `BackAppAddress` 到下载分区末尾的可用空间限制，使用前应修正边界检查。
- 当前仅恢复原工程代码，未完成独立工程编译和硬件传输验证。
