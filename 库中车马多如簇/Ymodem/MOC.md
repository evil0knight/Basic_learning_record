# Ymodem

[← 模块总览](../MOC.md) | [← 主页](../../index.md) | [← Ymodem传输协议](../../术中自有万钟粟/OTA/Ymodem传输协议.md)

---

| 文件                                      | 作用                                                   |
| ----------------------------------------- | ------------------------------------------------------ |
| [ymodem.h](./ymodem.h) / [ymodem.c](./ymodem.c) | Ymodem 协议（接收/发送/CRC），数据经 Sink 回调交给上层 |
| [common.h](./common.h) / [common.c](./common.c) | 工具函数 + UART Port 字节收发                          |
| [ymodem_config.h](./ymodem_config.h)         | UART 实例编号、超时轮询次数                            |

## 移植

1. 放入目标工程：

```text
03_Middlewares/communication/ymodem/
├── ymodem.c/.h
├── common.c/.h
└── ymodem_config.h
```

2. 同时移植 [UART Port](../UART/MOC.md)（`core_usart_*`）。
3. 在 `ymodem_config.h` 配置 UART 实例编号。
4. 将 `ymodem.c`、`common.c` 加入编译。

## 接收等待与平台钩子

`Receive_Byte()` 采用轮询 `SerialKeyPressed()` 的方式等待单字节，最多执行 `YMODEM_BYTE_TIMEOUT_COUNT` 次；达到上限返回超时并终止当前会话。该宏位于 [ymodem_config.h](./ymodem_config.h)，应结合主频、波特率和 UART 驱动一次轮询耗时实测配置，不能直接照搬默认值。

每 1024 次轮询调用一次 `YMODEM_POLL_HOOK()`。默认是空宏，产品可将它映射为看门狗喂狗、RTOS 让步、DMA 状态维护或超时计数。钩子必须快速返回，不应执行 Flash 擦写或再次等待 UART。
