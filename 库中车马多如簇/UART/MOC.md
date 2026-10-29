# UART(串口)

[← 返回 MOC](../MOC.md) | [← 主页](../../index.md)

[← OTA](../../术中自有万钟粟/OTA/MOC.md)|[← RTOS 架构 MCU 层](../代码架构/firmware/04_platform/platform_mcu/MOC.md) | [← 裸机架构 MCU 层](../代码架构/frimware_bare/04_platform/platform_mcu/MOC.md)

> [下载串口驱动](https://pan.baidu.com/s/1OfEdpC5rkB4MYNhvWFjdkQ?pwd=4444)
>
> v1:2026/10/28,`01_app` 承接 HAL 中断入口，`04_platform` 提供 注册、uart对象表挂载回调函数,

## 目录

| 目录                                                                  | 作用                                                   |
| --------------------------------------------------------------------- | ------------------------------------------------------ |
| [01_app/app_isr_handlers](./01_app/app_isr_handlers/user_isr_handlers.c) | HAL UART 中断回调入口，转发到 USART Port               |
| [04_platform/platform_mcu/uart](./04_platform/platform_mcu/uart/MOC.md)  | `core_usart_*` 接口 + HAL 分发 + DMA + 回调 + 静态表 |
| [知识](./知识/MOC.md)                                                    | UART 协议、波特率、电平、收发写法等笔记                |

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

`huart1` 必须由 CubeMX 生成的 `MX_USART1_UART_Init()` 完成硬件初始化,
这里是把某个uart的所有信息打包成一个对象,对象多了,所以维护一个对象数组存放所有的uart对象
这里不挂载回调函数,看下面的回调函数注册,再其他地方注册进来

2. 从底往上看,[app_isr_handler](01_app\app_isr_handlers\user_isr_handlers.c)里,写了回调入口,所有的回调先到这里

   ``HAL_UART_RxCpltCallback``回调函数是普通cpu接收,搬运依靠cpu搬运到缓冲区,缓冲区满了就中断
   ``HAL_UART_TxCpltCallback``回调函数是cpu发送回调和DMA发送回调共用的,搬运到uart外设以后触发回调,uart外设自己发
   ``HAL_UART_ErrorCallback``回调函数是只要发送或者接收,检测到错误就会触发
   ``HAL_UARTEx_RxEventCallback``回调函数是接受DMA空闲IDLE,半满HT,全满TC就会触发,
   发送DMA半满HT,全满TC就会触发,启动DMA的时候是可以设置关闭某个的(比如关闭半满)

   ``HAL_UART_RxCpltCallback``,``HAL_UART_TxCpltCallback``,``HAL_UART_ErrorCallback``,``HAL_UARTEx_RxEventCallback``
   这些全部都是hal层已经定义的[弱函数](..\..\书中自有黄金屋\C语言\__attribute__.md)(这里有__weak还有相关的解释:
   通过弱函数的方式,能实现下层调用上层函数且不包含上层的头文件,只要在下层弱定义一个,然后上层再定义一下就好了)
   这些回调函数是多个huart句柄共用的
3. 进入这里以后,[app_isr_handler](01_app\app_isr_handlers\user_isr_handlers.c)这个文件算上层,所以直接包含了platform层的uart_port文件,
   然后回调函数里直接调用platform层写好的 `core_usart_irq_rx_complete`,`core_usart_irq_tx_complete`,`core_usart_irq_error`,`core_usart_irq_rx_event`函数,

   [usart_port_config.h](04_platform\platform_mcu\uart\inc\usart_port_config.h)上面有个表,用来注册uart实例和对应的回调函数:
   这个表把huart句柄和他的回调,波特率等等注册在一起,**回调进来根据句柄调用对应的回调函数**
4. 怎么注册的把句柄和实例和回调函数在一起的?

   在user_init的 `userTaskInitFunction`或者在裸机的UserAppInit调用 `core_usart_init`.先清空所有的挂载
   然后回调函数是其他地方提供过来的,在service层初始化的时候,或者bsp的adapter层,调用core_usart_register_rx_callback等,挂载回调函数
   `core_usart_register_rx_callback`,`core_usart_register_tx_callback`,`core_usart_register_error_callback`,这些在
   里面是用队列等通知service层回调完成,或者包含platform层的.h文件,使用bsp传上来的回调函数
5. 开启:
   还有一些发送和接收的函数,比如 `core_usart_receive_to_idle_dma`这个是启动一轮的DMA半满全满空闲接收,
