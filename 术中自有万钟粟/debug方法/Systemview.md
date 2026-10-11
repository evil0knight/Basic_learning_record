# Systemview

[← 返回 debug方法](./MOC.md) | [← 主页](../../index.md)

> SEGGER Systemview 实时事件录制与可视化

---

## 原理

和[EasyLogger](EasyLogger.md)不同，SystemView 不会打包日志，因为这会增加运行时算力开销,相对于EasyLogger一个日志要50个字节,这个可能只要2到4个字节

他会放一帧数据到[RTT](RTT/MOC.md)缓冲区:

| 字段     | 说明                                           |
| -------- | ---------------------------------------------- |
| 事件类型 | 标识任务切换、中断进入/退出、任务创建等事件    |
| 时间戳   | 记录事件发生的时间，使用类似 LEB128 的变长编码 |
| 对象标识 | 标识任务、中断或其他被跟踪对象                 |
| 事件参数 | 根据事件类型记录任务优先级、状态等附加信息     |

对于时间戳,使用了一种类似LEB128的变长压缩算法,如果时间差很小,时间戳只占一个字节,如果差的很大,才用到两个字节

### 那他是怎么知道任务切换,中断切换的呢?

* 他利用了RTOS提供的跟踪宏接口和钩子函数（`traceTASK_CREATE`、`traceTASK_SWITCHED_IN`、`traceTASK_SWITCHED_OUT`）
  当把SystemView 加进工程时，SystemView 的头文件（SEGGER_SYSVIEW_FreeRTOS.h）会暴力覆盖这个宏

### 那他是怎么知道时间的呢?

* 不同于Easylogger,他直接读ARM Cortex-M 内核里的硬件寄存器:[DWT](DWT.md)Cycle Counter 计数器或者[systick定时器](../Cortex-M4内核原理/systick.md)

### 那这些帧里的任务地址是怎么翻译成任务名的呢?

* 这是因为在刚跑起来初始化或者空闲任务的时候,SystemView会发送给电脑一些长的数据包,这些数据包里记录了这些地址对应的函数名

## 效果

SystemView 将这些事件数据以图形化的方式展现，为后续调整任务优先级和分析程序调用链提供依据。

## 移植教程:
