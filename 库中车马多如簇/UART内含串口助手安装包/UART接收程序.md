# UART接收程序

[← 返回 MOC](MOC.md) | [← 主页](../../index.md)

### [volatile笔记](../../书中自有黄金屋/C++PrimerPlus/volatile.md)

---

## 配置问题

在选择写法前，请回答以下问题（一次性全部回答）：

1. 需要几路 UART 接收？（如：UART1 + UART3 共2路）
2. RX 缓冲区大小？（字节，RAM 紧张建议 32~64，宽裕建议 128~256）
3. 帧结束判断方式？（固定帧尾字符 如`\n` / 固定长度 / 超时空闲）

---

## 小内存,快反应,少数据情况的UART程序:

**UART_RX_ISR(void)**:依靠波特率时钟将跳变一位一位的放到移位寄存器中去，满8位就放到接收数据寄存器中，RXNE（接收寄存器非空）标志位置为 1，触发接收中断。

```
// 共享变量须声明为 volatile，防止编译器将其优化为寄存器缓存
volatile unsigned char rx_buffer[50];	//缓冲区
volatile unsigned char rx_index = 0;	// 写索引
volatile unsigned char rx_flag = 0;    // 帧接收完成标志

// UART 接收中断服务函数
// 硬件每接收完 1 字节（RXNE 置位）触发一次
void UART_RX_ISR(void) {
    if (UART_GetITStatus(UART_RX_IT) != RESET) {//如果接收了1B
        // UART_ReceiveData() 本质是读接收数据寄存器，直接操作寄存器写法：USART1->RDR（STM32）/ UDR0（AVR）
        unsigned char data = UART_ReceiveData();
  
        if (rx_index < 50) {
            rx_buffer[rx_index++] = data;

            // 假设以 '\n' 作为帧结束符
            if (data == '\n') {
                rx_flag = 1;
            }
        } else {
            // 缓冲区溢出：未收到帧结束符，丢弃当前帧，重置索引
            rx_index = 0;
        }
    }
}

int main(void) {
    UART_Init();
  
    while(1) {
        if (rx_flag == 1) {
            Process_Data(rx_buffer, rx_index);

            // 关中断保护临界区，防止复位过程中 ISR 写入导致 rx_index 错误清零
            UART_DisableIRQ();// STM32: UART->CR1 &= ~USART_CR1_RXNEIE  |  51: ES = 0
            rx_index = 0;
            rx_flag = 0;
            UART_EnableIRQ();// STM32: UART->CR1 |= USART_CR1_RXNEIE   |  51: ES = 1
        }
    }
}
```

---

## 高频率打断,数据包长度不固定,主程序高优先级

UART_DMA_Init：开启空闲中断，并以循环模式启动 DMA 接收。DMA 在后台自动将 RDR 中的数据搬入 rx_buffer，写指针到达末尾后自动回绕，CPU 全程不参与搬运。

**USART1_IRQHandler**：一帧完毕，因为两帧之间有一段间隔（总线静默超过1个字符时间，随波特率变化），时间一到，UART硬件逻辑判定总线空闲，触发空闲中断，将 rx_packet_ready 置 1 通知主循环处理。主循环用读指针追赶 DMA 写指针来消费数据。

```
#define RX_BUF_SIZE 256  // 建议为最大单帧长度的 2 倍以上

// rx_buffer 由 DMA 写入，须声明为 volatile
// read_ptr 仅在主循环中访问，无需 volatile
volatile uint8_t rx_buffer[RX_BUF_SIZE];
volatile uint8_t rx_packet_ready = 0;  // 帧接收完成标志（IDLE 中断置位）
uint16_t read_ptr = 0;                 // 环形缓冲区读指针

// 初始化 DMA 循环接收，并使能 IDLE 中断
void UART_DMA_Init(void) {
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    // DMA 循环模式：写指针到达末尾后自动回绕，无需手动重启
    HAL_UART_Receive_DMA(&huart1, (uint8_t*)rx_buffer, RX_BUF_SIZE);
}

// USART1 中断服务函数
// 先调用 HAL 库处理其他中断源，再单独处理 IDLE 标志
void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);

    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        rx_packet_ready = 1;
    }
}

// 在主循环中调用，以追赶方式读取环形缓冲区中的新数据
void Process_UART_Data(void) {
    if (rx_packet_ready == 0) return;

    // write_ptr：DMA 当前写入位置 = 总长度 - 剩余计数
    //__HAL_DMA_GET_COUNTER 读的是 DMA 的剩余传输计数寄存器（NDTR），它的值是"还剩多少个字节没搬"
    uint16_t write_ptr = RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);

    while (read_ptr != write_ptr) {
        uint8_t data = rx_buffer[read_ptr];

        // 协议解析（帧头校验、CRC 等）

        if (++read_ptr >= RX_BUF_SIZE)
            read_ptr = 0;  // 环形回绕
    }

    rx_packet_ready = 0;
}
```
