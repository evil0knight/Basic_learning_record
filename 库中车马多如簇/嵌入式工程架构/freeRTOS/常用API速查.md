# 常用API速查

[← 返回 MOC](MOC.md) | [← 相关原理](../../../书中自有黄金屋/操作系统/MOC.md#os-sync) | [← 主页](../../../index.md)

---

| API | 作用 |
| --- | ---- |
| `xTaskCreate()` | 动态创建任务 |
| `xTaskCreateStatic()` | 静态创建任务 |
| `vTaskDelete()` | 删除任务 |
| `vTaskDelay()` | 相对延时 |
| `vTaskDelayUntil()` | 周期对齐延时 |
| `xQueueCreate()` | 创建队列 |
| `xQueueSend()` | 发队列消息 |
| `xQueueReceive()` | 收队列消息 |
| `xSemaphoreGive()` | 释放信号量 |
| `xSemaphoreTake()` | 获取信号量 |
| `xTimerCreate()` | 创建软件定时器 |
| `xTimerStart()` | 启动软件定时器 |
| `xQueueSendFromISR()` | 中断里发队列消息 |
| `xSemaphoreGiveFromISR()` | 中断里释放信号量 |
