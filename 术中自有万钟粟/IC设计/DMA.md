# DMA：

[← 返回 MOC](MOC.md) | [← 主页](../../index.md)

---

## DMA到底解决了什么

DMA（Direct Memory Access）解决的是：**大量数据搬运不该占用 CPU 指令周期**。

没有 DMA 时，CPU 需要反复做：

1. 读外设数据寄存器
2. 写内存缓冲区
3. 循环直到传输结束

问题：

- CPU 被“搬运工工作”占满，主循环和控制任务被挤压
- 高频采样/通信下容易丢数据
- 中断频率高，系统抖动明显

用了 DMA 后：

- CPU 只负责“配置一次 + 完成后处理”
- 数据搬运由 DMA 硬件自动完成
- 典型收益：更稳的吞吐、更低的 CPU 占用

适用高频数据流场景：ADC 连续采样、UART/SPI/I2C 批量收发、DAC 连续输出。

---

## DMA为什么能提升性能

先明确：DMA提升的是**系统效率**，不是让单次内存访问变快。

核心原因：

1. **CPU 从字节级参与变为块级参与**原来每个数据都要 CPU 管；现在 CPU 只在半传输/全传输中断时处理一块数据。
2. **并行性提高**DMA 在搬数据时，CPU 可以同时运行算法、状态机、通信协议栈。
3. **中断压力可控**
   用缓冲区 + 半传输/全传输事件替代“每个样本一次中断”，调度更稳定。

工程上常见结果：

- CPU 占用下降
- 丢包/丢样概率下降
- 实时任务更容易满足周期要求

注意代价：DMA也占总线带宽。若配置不当（优先级、块大小、宽度），会造成总线竞争。

---

## CPU/Cache/Memory/Device

把数据路径看成四层：

### 1) Device（外设）

数据生产者/消费者，例如 ADC、UART、SPI。

### 2) Memory（内存）

SRAM 作为环形缓冲区或帧缓冲区。

### 3) DMA（搬运通道）

在 Device 和 Memory 之间做自动传输：

- 方向：P2M / M2P / M2M
- 粒度：8/16/32 位
- 地址模式：固定 / 自增
- 模式：Normal / Circular

### 4) CPU（控制与计算）

不再逐点搬运，只做：

- 初始化 DMA
- 响应 HT/TC 中断
- 对缓冲区做滤波、解码、控制算法

### Cache 说明

- 在很多入门 MCU（如 Cortex-M3/M4 部分型号）中，可能没有数据 Cache，DMA 一般与 SRAM 直接交互。
- 在带 D-Cache 的 MCU（如部分 Cortex-M7）中，要额外处理 Cache 一致性（clean/invalidate），否则会出现“DMA 数据已更新但 CPU 读到旧值”。

一句话理解整条链路：

**Device 产出数据 -> DMA 搬到 Memory -> CPU 按块处理数据。**

这就是 DMA 的正确分工。

## DMA伪代码

```text
void DMA_SPI_ADC_Init(void) {
    // 配置DMA通道
    DMA_InitStructure.Channel = DMA_CHANNEL_X;
    DMA_InitStructure.PeripheralBaseAddr = (uint32_t)&SPIx->DR; // SPI数据寄存器
    DMA_InitStructure.MemoryBaseAddr = (uint32_t)adc_buffer;   // 内存缓冲区
    DMA_InitStructure.Direction = DMA_PERIPH_TO_MEMORY;        // 外设到内存
    DMA_InitStructure.NbData = ADC_BUFFER_SIZE;                // 传输长度
    DMA_InitStructure.Priority = DMA_PRIORITY_HIGH;            // 高优先级
    DMA_Init(DMAx, &DMA_InitStructure);
  
    // 使能DMA中断
    DMA_ITConfig(DMAx, DMA_IT_TC, ENABLE);
  
    // 启动DMA
    DMA_Cmd(DMAx, ENABLE);
}

// DMA传输完成中断
void DMAx_IRQHandler(void) {
    if (DMA_GetITStatus(DMAx, DMA_IT_TC)) {
        // 数据搬运完成，处理adc_buffer中的数据
        ProcessADCData(adc_buffer);
        // 重新启动DMA，准备下一次传输
        DMA_Cmd(DMAx, ENABLE);
        DMA_ClearITPendingBit(DMAx, DMA_IT_TC);
    }
}
```
