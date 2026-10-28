# USART Port

[← UART 模块](../../../MOC.md) | [← 主页](../../../../../index.md)

## 文件

| 文件                                                              | 作用                                            |
| ----------------------------------------------------------------- | ----------------------------------------------- |
| [usart_port.h](./inc/usart_port.h) / [usart_port.c](./src/usart_port.c) | `core_usart_*` 接口 + HAL 分发 + DMA + 回调桥 |
| [usart_port_config.h](./inc/usart_port_config.h)                     | 槽位表（HAL 句柄 + 波特率 + 超时）              |

HAL 中断回调入口位于
[user_isr_handlers.c](../../../01_app/app_isr_handlers/user_isr_handlers.c)，
Port 只提供 `core_usart_irq_*` 分发接口。

## port文件的使用

1. 在 `inc/usart_port_config.h` 中配置逻辑实例：

```c
static const st_usart_config_t g_usart_configs[CORE_USART_MAX] = {
    [CORE_USART1] = {
        .p_handle = &huart1,
        .baudrate = 115200U,
        .timeout = CORE_USART_DEFAULT_TIMEOUT
    },
};
```

`huart1` 必须由 CubeMX 生成的 `MX_USART1_UART_Init()` 完成硬件初始化。

3. 在 server 线程开始时注册回调并启动接收：

这里就是再回调函数直接运行这个 ``server_uart_rx``回调函数,然后他就会处理了,处理了以后就发送队列通知线程或者什么逻辑就自己写了,这里都是service层的了,

```c
static uint8_t s_server_rx_buf[128];//DMA搬运缓冲区

static void server_uart_rx(uint8_t *data, uint16_t size)
{
    //处理本次接收数据
    if ((data != NULL) && (size > 0U)) {
        server_protocol_parse(data, size);
    }
    //启动第二次DMA接收
    core_usart_receive_to_idle_dma(CORE_USART1,
                                   s_server_rx_buf, 
                                   sizeof(s_server_rx_buf));
}

void server_thread(void *argument)
{
    (void)argument;
    //主次回调函数-->自己写的处理函数
    core_usart_register_rx_callback(CORE_USART1, server_uart_rx);
    //启动第一次接收DMA
    core_usart_receive_to_idle_dma(
        CORE_USART1, s_server_rx_buf, sizeof(s_server_rx_buf));

    for (;;) {
        server_thread_process();//线程逻辑
    }
}
```

线程开始时注册回调并启动首轮接收。UART 空闲线或 DMA 缓冲区接收满时执行回调，`size` 为本次实际接收长度；回调结束后重新启动下一轮接收。
