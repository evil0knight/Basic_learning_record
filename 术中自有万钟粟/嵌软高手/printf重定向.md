# printf 重定向

[← 嵌软高手知识地图](./MOC.md) | [← 主页](../../index.md)

---

printf 默认输出到调试器（半主机模式），MCU 独立运行时需要用 microlib + fputc 重定向到实际外设。

两种常用方式：

| 方式 | 输出目标    | 速度             | 需要硬件            | 笔记                                                                      |
| ---- | ----------- | ---------------- | ------------------- | ------------------------------------------------------------------------- |
| UART | 串口助手    | 受波特率限制     | USB转TTL / 板载串口 | [UART printf配置](../../库中车马多如簇/UART内含串口助手安装包/printf配置.md) |
| RTT  | J-Link 终端 | 极快（SRAM直写） | J-Link 调试器       | [RTT printf配置](../配置学习/RTT/使用.md)                                    |

---

## 前提

打开 [microlib](./microlib打开原因.md)，否则 printf 走半主机模式会导致程序卡死。

---

## UART 重定向

在 `usart.c` 中实现 `fputc`，调用 `HAL_UART_Transmit` 逐字节发送。

详见 [UART printf配置](../../库中车马多如簇/UART内含串口助手安装包/printf配置.md)

---

## RTT 重定向

在 `usart.c` 中实现 `fputc`，调用 `SEGGER_RTT_PutChar` 写入 RTT 缓冲区。

详见 [RTT使用](../配置学习/RTT/使用.md)

---

> 💡 需要格式输出（彩色分级、日志过滤、按标签存储）看 [EasyLogger](../配置学习/EasyLogger.md)，使用教程将在此更新。
