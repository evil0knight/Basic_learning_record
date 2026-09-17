# Ymodem

[← 模块总览](../MOC.md) | [← 主页](../../index.md)

## 文件

| 文件                                      | 作用                                |
| ----------------------------------------- | ----------------------------------- |
| [ymodem.h](./ymodem.h) / [ymodem.c](./ymodem.c) | Ymodem 协议、收发、CRC 和 Sink 回调 |
| [common.h](./common.h) / [common.c](./common.c) | 协议工具函数                        |
| [ymodem_config.h](./ymodem_config.h)         | UART 实例和收发模式配置             |

## 移植步骤

1. 复制 `ymodem.c/.h`、`common.c/.h` 和 `ymodem_config.h`，加入目标工程编译。
2. 配置 UART Port 资源。

   ```c
   static const st_usart_config_t g_usart_configs[CORE_USART_MAX] =
   {
       [CORE_USART1] = {
           .handle = &huart1,
           .baudrate = 115200U,
           .timeout = CORE_USART_DEFAULT_TIMEOUT
       }
   };
   ```
   阻塞模式配置 UART、GPIO、时钟并调用 `MX_USART1_UART_Init()`；DMA 模式还需配置 DMA 接收通道和 UART 空闲线中断。
3. 选择 Ymodem 接收模式。

   ```c
   #define YMODEM_RECEIVE_MODE_BLOCKING  0U
   #define YMODEM_RECEIVE_MODE_DMA       1U
   #define YMODEM_RECEIVE_MODE           YMODEM_RECEIVE_MODE_BLOCKING
   ```
   Bootloader 使用阻塞模式；带 RTOS 的 OTA App 使用 DMA 模式时改为 `YMODEM_RECEIVE_MODE_DMA`。
4. DMA + RTOS 模式配置信号量。

   ```c
   typedef enum
   {
       USER_SEMA_IDX_YMODEM_RX,
       USER_SEMA_IDX_MAX
   } user_sema_idx_t;

   user_sema_config_t g_user_semaphores[USER_SEMA_IDX_MAX] =
   {
       [USER_SEMA_IDX_YMODEM_RX] =
       {
           true, true, 1U, 0U, NULL
       }
   };
   ```
   统一资源初始化后获取句柄：

   ```c
   osal_sema_handle_t ymodem_rx_semaphore =
       user_sema_get(USER_SEMA_IDX_YMODEM_RX);
   ```
   DMA 回调释放该信号量，Ymodem 接收函数等待该信号量。Bootloader 阻塞模式不配置、不获取信号量。
5. DMA 模式提供缓冲区并注册 RX 回调；阻塞模式不注册回调。

   ```c
   static uint8_t s_ymodem_rx_buffer[1024];

   core_usart_register_rx_callback(CORE_USART1, ymodem_rx_callback);
   core_usart_receive_to_idle_dma(
       CORE_USART1, s_ymodem_rx_buffer, sizeof(s_ymodem_rx_buffer));
   ```
   回调保存接收长度并释放 `USER_SEMA_IDX_YMODEM_RX`。阻塞模式由 Ymodem 直接调用 `core_usart_receive()`。
6. 注入读写函数和 Flash Sink。

   ```c
   Ymodem_SetIo(ota_adapter_uart_read, ota_adapter_uart_write);
   Ymodem_ReceiveWithSink(
       s_ymodem_packet_buffer,
       ymodem_flash_sink,
       sink_context,
       OTA_APP_ADDRESS);
   ```
   Sink 负责 Flash 写入、地址递增、最后数据块裁剪和错误返回。详见 [OTA MOC](../OTA/MOC.md) 和 [Flash MOC](../Flash/MOC.md)。
