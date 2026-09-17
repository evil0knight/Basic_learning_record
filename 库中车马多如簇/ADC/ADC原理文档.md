# ADC 原理文档

[← 返回 MOC](./MOC.md) | [← 主页](../../index.md)

## 数据流

模拟输入经过采样保持和量化，转换结果写入 ADC 数据寄存器。DMA 模式下，
DMA 把数据寄存器内容搬到内存；非 DMA 模式下，ADC 转换完成中断通知软件
读取数据。模块再用轮询测量 `core_adc_measure()` 覆盖单点采样的场景。

## 双缓冲

模块维护两块等长缓冲区，外设只写 `s_adc_buffer[s_adc_write_index]`，
另一块靠产生位标记为待处理块，采样任务按位里带的块号去读。一轮搬运
完成后切一次写入下标，写入块变成待处理块，刚读完的块变成新的写入块。
采样任务读数据时外设正在写另一块，因此读到的永远是完整的一轮数据。

```text
写入块:  buf0  |  buf1  |  buf0  |  buf1  | ...
待处理块: buf1  |  buf0  |  buf1  |  buf0  | ...
         一轮结束切换一次写入下标
```

## 事件通道

事件用任务通知传递，不占额外的内核对象：通知位存在目标任务自己的
通知值里，所以一个事件只能有一个接收任务，正好对应这里的三个方向。

| 通知位 | 发送方 | 接收方 | 含义 |
| --- | --- | --- | --- |
| `ADC_BIT_DONE` | 中断回调 | `adc_transfer_task` | 一轮搬运完成 |
| `ADC_PRODUCED(x)` | `adc_transfer_task` | `adc_sample_task` | 第 x 块已写好 |
| `ADC_CONSUMING(x)` | `adc_sample_task` | `adc_transfer_task` | 第 x 块正在被消费 |

产生位和消费位在各自任务的通知值里编号相同，互不干扰。中断里只做一次
`osal_task_notify_from_isr()`，把耗时处理全部留给任务。

## 任务分工

`adc_transfer_task` 等 `ADC_BIT_DONE`，通知采样任务产生位，然后换下写入块；
`adc_sample_task` 等产生位，取互斥量后调用 `adc_user_process()` 做滤波、
换算或上报，处理完清掉产生位。搬运任务优先级高于采样任务，保证切换及时。

## 一拍的完整时序

```text
DMA 写完 buf0
  → 中断：通知搬运任务 ADC_BIT_DONE
  → 搬运任务：通知采样任务 PRODUCED(0)，换写入块为 buf1，启动下一轮搬运
  → 采样任务：取互斥量，通知搬运任务 CONSUMING(0)，处理 buf0，
              清 PRODUCED(0)，放互斥量
DMA 写完 buf1
  → 搬运任务：通知 PRODUCED(1)，换写入块为 buf0；
              如果 PRODUCED(0) 还在（buf0 没消费完），等 CONSUMING(0)，
              再用互斥量等采样任务真的处理完，才往 buf0 写
```

## DMA 与中断

DMA 模式适合固定频率、多通道和连续采样，CPU 负担低；中断模式适合低
采样率或没有可用 DMA 通道的场景。区别只在 `core_adc_start()` 里调
`HAL_ADC_Start_DMA()` 还是 `HAL_ADC_Start_IT()`，以及 CubeMX 里在 NVIC
勾的是 ADC 的 DMA 流中断还是 ADC 中断。两种情况下 `stm32f4xx_it.c` 的
中断入口都是 CubeMX 生成的，只调 `HAL_DMA_IRQHandler()` 或
`HAL_ADC_IRQHandler()`，最后都进同一个 `HAL_ADC_ConvCpltCallback()`，
上层任务不需要感知搬运方式。

## 互斥量边界

互斥量只做互斥，不做事件，且不在中断里获取，它挡住一种情况：采样任务
还在读 buf0，搬运任务已经转到 buf0 准备写入，DMA 会覆盖正在被读的数据。

`adc_sample_task` 从取到互斥量到处理完一直持有；`adc_transfer_task` 看到
下一块还有产生位时，先等消费位，再 `take` + `give` 一次互斥量，用这次
加解锁等采样任务真的处理完才启动下一轮。采样任务处理时间过长会推迟
下一轮搬运启动，因此业务处理要短，需要落盘或上报时交给其他任务。
