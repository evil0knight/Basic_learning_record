# ADC

[← 返回 MOC](../MOC.md) | [← 主页](../../index.md)
[← 返回 电流采样与处理](../../术中自有万钟粟/FOC/电流采样与处理.md)

> V1:2026/10/28:`01_app` 承接 HAL 中断入口，`04_platform` 提供 注册、uart对象表挂载回调函数+service双任务双缓冲版本(手敲)

## 文件

| 文件                                                                                                                                  | 作用                                                |
| ------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------- |
| [user_isr_handlers.h](./01_app/app_isr_handlers/user_isr_handlers.h) / [user_isr_handlers.c](./01_app/app_isr_handlers/user_isr_handlers.c) | App：HAL 回调入口，仅转发 Platform                  |
| [adc_port.h](./04_platform/platform_mcu/adc/inc/adc_port.h) / [adc_port.c](./04_platform/platform_mcu/adc/src/adc_port.c)                   | Platform：`core_adc_*` 接口、HAL 封装及事件分发   |
| [adc_port_config.h](./04_platform/platform_mcu/adc/inc/adc_port_config.h)                                                                | ADC 实例、HAL 句柄、超时；DMA/中断由调用接口区分    |
| [service_adc.h](./02_service/service_adc/service_adc.h) / [service_adc.c](./02_service/service_adc/service_adc.c)                           | Service：双缓冲状态、通知位、搬运任务和采样处理任务 |
|                                                                                                                                       |                                                     |

## port文件的使用

1. 在 `inc/adc_port_config.h` 中配置逻辑实例：

```c
static const st_adc_config_t g_adc_configs[CORE_ADC_MAX] = {
    [CORE_ADC1] = {
        .handle = &hadc1,
        .timeout = CORE_ADC_DEFAULT_TIMEOUT
    },
};
```

`hadc1` 必须由 CubeMX 生成的 `MX_ADC1_Init()` 完成硬件初始化,
这里是把某个adc的所有信息打包成一个对象,对象多了,所以维护一个对象数组存放所有的adc对象
这里不挂载回调函数,看下面的回调函数注册,再其他地方注册进来

2. 从底往上看,[app_isr_handler](01_app\app_isr_handlers\user_isr_handlers.c)里,写了回调入口,所有的回调先到这里

   ``HAL_ADC_ConvCpltCallback``回调函数是一轮转换完成,DMA模式下搬运完指定数量的数据后触发,普通中断模式下由转换完成事件触发(数据要CPU自己读,当前没做逐点搬运)
   ``HAL_ADC_ErrorCallback``回调函数是只要转换或者DMA,检测到错误就会触发
   ``HAL_ADC_ConvHalfCpltCallback``回调函数是DMA半满HT时触发,当前模块没实现,需要时再加,启动DMA的时候是可以设置关闭某个的(比如关闭半满)

   ADC没有UART的空闲IDLE接收事件

   ``HAL_ADC_ConvCpltCallback``,``HAL_ADC_ErrorCallback``,``HAL_ADC_ConvHalfCpltCallback``
   这些全部都是hal层已经定义的[弱函数](..\..\书中自有黄金屋\C语言\__attribute__.md)(这里有__weak还有相关的解释:
   通过弱函数的方式,能实现下层调用上层函数且不包含上层的头文件,只要在下层弱定义一个,然后上层再定义一下就好了)
   这些回调函数是多个hadc句柄共用的,靠传进来的句柄区分是哪个adc
3. 进入这里以后,[app_isr_handler](01_app\app_isr_handlers\user_isr_handlers.c)这个文件算上层,所以直接包含了platform层的adc_port文件,
   然后回调函数里直接调用platform层写好的 `core_adc_irq_conv_complete`,`core_adc_irq_error`函数,

   [adc_port_config.h](04_platform\platform_mcu\adc\inc\adc_port_config.h)上面有个表,用来注册adc实例和对应的HAL句柄,这里不挂载回调函数:
   这个表把hadc句柄、超时等等注册在一起,**回调进来根据句柄调用对应的回调函数**
4. 怎么注册的把句柄和实例和回调函数在一起的?

   在user_init的 `userTaskInitFunction`或者在裸机的UserAppInit调用 `core_adc_init`.先清空所有的挂载
   然后回调函数是其他地方提供过来的,在service层初始化的时候(当前是 `service_adc_start` 里注册的),或者bsp的adapter层,调用core_adc_register_conv_callback等,挂载回调函数
   `core_adc_register_conv_callback`,`core_adc_register_error_callback`,这些在
   里面是用队列等通知service层回调完成,或者包含platform层的.h文件,使用bsp传上来的回调函数

5. 开启:
   `core_adc_receive_dma` 把指定数量的ADC转换结果DMA搬到缓冲区,搬完触发回调,Normal模式要再调一次才能启动下一轮
   `core_adc_stop_dma` 停止DMA采集
   中断方式用 `core_adc_receive_it` / `core_adc_stop_it`,在完成回调里调 `core_adc_get_value` 让CPU读结果
   两种方式通过不同接口选,不用宏切换

## Service 双缓冲职责

一直采样,这部分是 RTOS 版本；裸机只接入 App 和 Platform，再由主循环消费事件，不使用以下任务和互斥量。

1. 维护两块等长采样缓冲区，用读写下标区分写入块和完成块。
2. 中断通知搬运任务完成位，搬运任务通知采样任务产生位。
3. `service_adc_transfer_task` 切换并搬运的task,换写入块，必要时等消费位再启动下一轮。
4. `service_adc_sample_task` 切换并消费的task,取互斥量后消费已完成缓冲区。
5. 两个任务和互斥量从统一资源表取，事件用 OSAL 任务通知。
