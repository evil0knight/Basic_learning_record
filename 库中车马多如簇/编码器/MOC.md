# 编码器

[← 返回模块总表](../MOC.md) | [← 主页](../../index.md)

---

> 记录正交增量编码器的原理与 STM32 TIM 编码器模式驱动实现。
>
> 代码来源工程：[Intelligent_Agricultural_Equipment_Innovation_Competition](https://github.com/evil0knight/Intelligent_Agricultural_Equipment_Innovation_Competition)（`Mycode/encoder.c`）

## 笔记导航

| 主题 | 说明 | 笔记 |
| :---: | :---: | --- |
| 定时器编码器程序 | TIM 编码器模式读速、多路差分、三级滤波 | [定时器编码器程序](./定时器编码器程序.md) |

---

### 🔴工作原理

正交增量编码器输出 A、B 两相方波，相位差 90°。

- **计数**：A、B 两相各有上升沿和下降沿，STM32 TIM 编码器模式（`TI12`）对两路信号的四个边沿全部计数，即**四倍频**，每圈实际计数 = 编码器线数 × 4。
- **方向**：A 超前 B 为正转，B 超前 A 为反转，TIM 计数器自动递增或递减。

<!-- 在此插入 A/B 两相时序示意图 -->

---

### 🔴关键参数

**每圈总脉冲数**（含减速比）：

$$N = \text{线数} \times \text{倍频} \times \text{减速比}$$

**转速公式**（差分法，采样周期 $T_s$ 秒）：

$$\text{RPM} = \frac{\Delta cnt}{N} \times \frac{60}{T_s}$$

工程实例（采样周期 10 ms）：

$$\text{RPM} = \Delta cnt \times \frac{60}{0.01 \times 11 \times 4 \times 14} = \Delta cnt \times \frac{6000}{616}$$

| 参数 | 工程值 |
| :---: | :---: |
| 编码器线数 | 11 |
| 倍频 | 4（TI12 模式） |
| 减速比 | 14 |
| 采样周期 | 10 ms |
| 每圈脉冲数 | 616 |
