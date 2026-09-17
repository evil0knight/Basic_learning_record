# ADC Port

[← 返回 MOC](../MOC.md) | [← 主页](../../index.md)

## 文件

| 文件 | 作用 |
| --- | --- |
| [adc_port.h](./adc_port.h) / [adc_port.c](./adc_port.c) | `core_adc_*` 接口、HAL 分发、DMA/中断回调桥 |
| [adc_port_config.h](./adc_port_config.h) | ADC 实例、HAL 句柄、超时和 DMA 开关 |
| [adc_core.h](./adc_core.h) / [adc_core.c](./adc_core.c) | 双缓冲状态、通知位、搬运任务和采样处理任务 |
| [ADC移植文档.md](./ADC移植文档.md) | 工程接入、任务和互斥量申请 |
| [ADC原理文档.md](./ADC原理文档.md) | ADC、DMA、双缓冲、通知位和互斥量原理 |

## Port 文件作用

1. 封装 HAL ADC 轮询测量、启动和停止接口。
2. 槽位表保存逻辑实例与 ADC 句柄的对应关系。
3. 回调通过句柄反查实例，再调用上层注册的函数指针。
4. `ADC_PORT_USE_DMA` 切换 DMA 与 ADC 中断搬运。

## Port 文件使用

1. 在 [adc_port_config.h](./adc_port_config.h) 注册句柄：

```c
static const st_adc_config_t g_adc_configs[CORE_ADC_MAX] =
{
    [CORE_ADC1] = {
        .handle = &hadc1,
        .timeout = CORE_ADC_DEFAULT_TIMEOUT
    }
};
```

2. 注册回调并启动搬运：

```c
core_adc_init();
core_adc_register_conv_callback(CORE_ADC1, adc_conv_callback);
core_adc_start(CORE_ADC1, s_adc_buffer, ADC_CORE_BUFFER_SIZE);
```

3. 单点采样直接调轮询测量：

```c
uint16_t adc_value;
core_adc_measure(CORE_ADC1, &adc_value);
```

回调只报告完成事件，业务处理放在任务中执行。

## Core 文件作用

1. 维护两块等长采样缓冲区，用读写下标区分写入块和完成块。
2. 中断通知搬运任务完成位，搬运任务通知采样任务产生位。
3. `adc_transfer_task` 换写入块，必要时等消费位再启动下一轮。
4. `adc_sample_task` 取互斥量后消费已完成缓冲区。
5. 两个任务和互斥量从统一资源表取，事件用 OSAL 任务通知。
