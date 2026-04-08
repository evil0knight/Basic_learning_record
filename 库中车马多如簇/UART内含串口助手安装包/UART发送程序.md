# UART发送程序

[← 返回 MOC](MOC.md) | [← 主页](../../index.md)

---

## 配置问题

在选择写法前，请回答以下问题（一次性全部回答）：

1. 需要几路 UART 发送？（如：UART1 + UART2 共2路）
2. TX 缓冲区大小？（字节，仅 DMA 写法需要，建议与单次最大发送包长一致）
3. 单次最大发送数据量？（字节，用于判断是否需要 DMA）

---

## 阻塞式发送（简单场景）

UART_SendByte：向发送数据寄存器（TDR）写入1字节，硬件自动加起始位、停止位后串行移出。写入前须等待 TXE（发送寄存器空）标志置 1，否则会覆盖上一个未发完的字节。

```
// 发送单字节，阻塞等待发送寄存器就绪
// 直接操作寄存器写法：while(!(USART1->SR & USART_SR_TXE)); USART1->DR = data;
void UART_SendByte(unsigned char data) {
    while (UART_GetFlagStatus(UART_FLAG_TXE) == RESET);  // 等待 TXE 置 1
    UART_SendData(data);
}

// 发送字节数组
void UART_SendBuffer(unsigned char *buf, unsigned char len) {
    for (unsigned char i = 0; i < len; i++) {
        UART_SendByte(buf[i]);
    }
}

int main(void) {
    UART_Init();

    unsigned char msg[] = "hello\n";
    while(1) {
        UART_SendBuffer(msg, sizeof(msg) - 1);
        Delay_ms(1000);
    }
}
```

---

## 发送完成中断（无DMA的小单片机）

手动写第一个字节到 TDR 启动发送，硬件将其串行移出后置 TCF（发送完成）标志，触发中断。中断里写下一个字节，TCF 自动清零，循环直到发完整个数组。CPU 不阻塞，但每字节进一次中断。

```
volatile uint8_t tx_buf[10] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa};
volatile uint8_t tx_index = 0;

void user_isr(void) {
    if (TCEN && TCF) {          // 发送完成中断
        TCF = 1;                // 写1清零（该芯片特性）
        if (tx_index < 10) {
            URDATAL = tx_buf[tx_index++];
        } else {
            tx_index = 0;       // 发完，重置索引
        }
    }
}

int main(void) {
    UART_INITIAL();
    if (TXEF) {
        URDATAL = tx_buf[tx_index++];  // 写第一个字节触发发送，后续由中断驱动
    }
    while(1) { }
}
```

---

## DMA发送（高频率/大数据量场景）

UART_DMA_SendBuffer：将数据地址和长度交给 DMA，DMA 自动将内存数据逐字节搬入 TDR，CPU 不参与搬运。

HAL_UART_TxCpltCallback：DMA 搬完最后一字节后硬件触发 TC（传输完成）中断，HAL 库调用此回调清除 tx_busy，允许下一包发送。tx_busy 标志防止上一包未发完时再次启动 DMA 覆盖缓冲区。

```
volatile uint8_t tx_busy = 0;  // DMA 发送进行中标志

void UART_DMA_Init(void) {
    HAL_UART_Init(&huart1);
}

void UART_DMA_SendBuffer(uint8_t *buf, uint16_t len) {
    if (tx_busy) return;  // 上一包未发完，丢弃本次
    tx_busy = 1;
    HAL_UART_Transmit_DMA(&huart1, buf, len);
}

// DMA 传输完成中断回调（HAL 库自动调用）
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        tx_busy = 0;
    }
}

int main(void) {
    HAL_Init();
    UART_DMA_Init();

    uint8_t msg[] = "hello\n";
    while(1) {
        UART_DMA_SendBuffer(msg, sizeof(msg) - 1);
        HAL_Delay(1000);
    }
}
```
