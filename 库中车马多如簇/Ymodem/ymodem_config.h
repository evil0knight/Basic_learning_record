#ifndef YMODEM_CONFIG_H
#define YMODEM_CONFIG_H

/* UART logical index used by Ymodem. / Ymodem使用的UART逻辑编号。 */
#define YMODEM_UART_INDEX          0U

/* Receive byte timeout loop count; calibrate this value for the target board. */
/* 单字节接收超时轮询次数；需要按目标板主频校准。 */
#define YMODEM_BYTE_TIMEOUT_COUNT  0x100000UL

/* 单字节轮询期间的可选平台钩子；可用于喂狗、让出CPU或统计超时。 */
#ifndef YMODEM_POLL_HOOK
#define YMODEM_POLL_HOOK()         ((void)0)
#endif

#endif
