# 超声波

[← 返回模块总表](../MOC.md) | [← 主页](../../index.md)

---

> 记录 HC-SR04 超声波测距模块的原理与 STM32 输入捕获驱动实现。
>
> 代码来源工程：[Intelligent_Agricultural_Equipment_Innovation_Competition](https://github.com/evil0knight/Intelligent_Agricultural_Equipment_Innovation_Competition)（`Mycode/SR04.c`）

## 笔记导航

| 主题 | 说明 | 笔记 |
| :---: | :---: | --- |
| 输入捕获程序 | TIM 输入捕获测量回响时间，多路状态机 | [输入捕获程序](./输入捕获程序.md) |

---

### 🔴工作原理

TRIG 引脚输出 ≥10 μs 的高电平脉冲触发测量，模块随即发出 8 个 40 kHz 超声波脉冲并拉高 ECHO 引脚；超声波遇障碍物反射回来后 ECHO 变低，高电平持续时间即为声波往返时间 $t$。

$$d = \frac{v_s \cdot t}{2} \approx \frac{340\,\text{m/s} \times t}{2}$$

换算为 cm：$d\,(\text{cm}) \approx t\,(\mu\text{s}) \div 58$（工程中常用 57.5 微调）。

<!-- 在此插入时序示意图 -->

测量范围约 2 cm ~ 400 cm，盲区 < 2 cm 的数据需过滤。
