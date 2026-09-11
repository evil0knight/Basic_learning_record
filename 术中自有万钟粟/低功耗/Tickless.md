# Tickless（低功耗空闲模式）

[← 低功耗知识地图](./MOC.md) | [← 主页](../../index.md)

> **我的理解（精髓）：** 把正常的 SysTick 关掉，然后看延时链表，默认做法是把 SysTick 重装到 N ms 后一次性唤醒,或者把下一个要唤醒的时间写到 RTC / 低功耗定时器里倒计时，时间到了再中断唤醒，或者就事件唤醒。

---

## 实现逻辑

1. 所有任务阻塞：当前没有就绪任务，系统自动切入**空闲任务**运行
2. 空闲任务运行
3. 判断是否允许睡眠：`eTaskConfirmSleepModeStatus()`
   - 确认系统可否进入低功耗（若还有定时器/事件将唤醒，则禁止睡眠）
4. 停止 SysTick：防止每 1ms 的周期中断打断睡眠
   - 查阅延时链表，算出距下一个要唤醒任务还剩多少时间（期望睡眠时长 N ms）
   - `portNVIC_SYSTICK_CTRL_REG &= ~portNVIC_SYSTICK_ENABLE_BIT` 关闭周期中断
5. 设置 SysTick 为 N ms 后唤醒
   - `portNVIC_SYSTICK_LOAD_REG = ulReloadValue` 写重装值
   - `portNVIC_SYSTICK_CURRENT_VALUE_REG = 0` 清计数，从 0 重新数
6. 睡前钩子：`configPRE_SLEEP_PROCESSING(xIdleTime)`（GPIO 拉低、关外设、电源管理等自定义动作）
7. 默认休眠函数：`vPortSuppressTicksAndSleep(xExpectedIdleTime)` 按期望休眠时长执行策略
8. 执行睡眠：`__WFI()` 或 `HAL_PWR_EnterSTOPMode()`（或写入 RTC/LPTIM 倒计时后入睡）

## 唤醒

1. **非 Tickless 唤醒**
   - 依赖固定的 SysTick 周期中断（如每 1ms 一次）
   - 即便没有任务运行，CPU 也会被强制唤醒检查任务，无法长时间深睡，功耗较高
2. **Tickless 唤醒**
   1. 中断唤醒：低功耗定时器/RTC 到期中断，或外部硬件事件中断唤醒 CPU
   2. 唤醒后钩子：`configPOST_SLEEP_PROCESSING(xExpectedIdleTime)`（恢复外设、日志等）
   3. 判断是否为 SysTick 唤醒
   4. Tick 补偿：读取实际休眠经过时间，`vTaskStepTick(ulCompleteTickPeriods)` 一次补齐 OS 丢掉的 Tick，防止时间跳变
   5. 恢复 SysTick：`portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_ENABLE_BIT`
   6. 恢复调度：唤醒到期任务，继续执行业务逻辑
