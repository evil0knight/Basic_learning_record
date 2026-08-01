# PWM 与比较匹配中断

[← 中断总览](./MOC.md) | [← 主页](../../../index.md)

---

## CMP 比较匹配中断

定时器模块包含一个自由运行的计数器（CNT）和一个或多个比较寄存器（Compare Register，CMP）。当 CNT == CMP 时，硬件触发 **比较匹配事件**，可配置为产生中断。这是定时器中断（PWM、输出比较、输入捕获等）的通用底层机制。

```
计数器值 CNT
     │
     ▼
  ┌──────────────────────────────────────────┐
  │  0 ──────────────── CMP ──────── ARR     │
  │                      ↑                   │
  │               比较匹配，触发中断           │
  └──────────────────────────────────────────┘
```

- **ARR（Auto-Reload Register）**：定时器周期，CNT 到达 ARR 后归零（或反向计数）。
- **CMP / CCR（Capture/Compare Register）**：比较值，CNT 到达此值时触发事件或中断。
- 每个通道通常有独立的 CMP 寄存器，可产生独立的中断。

---

## 中心对齐模式下的双 CMP 中断（每通道两个比较点）

在四开关 Buck-Boost 等功率变换器中，PWM 采用**中心对齐（上下计数）模式**，每个通道配置两个比较寄存器：

```c
struct PWMDutyVariable
{
    bool     buckBoostMode = false;
    float    dutyA         = 0.0f;
    float    dutyB         = 0.0f;
    uint16_t ACMP1         = 8000;   // 通道A — 上升沿阶段触发
    uint16_t ACMP3         = 8000;   // 通道A — 下降沿阶段触发
    uint16_t BCMP1         = 8000;   // 通道B — 上升沿阶段触发
    uint16_t BCMP3         = 8000;   // 通道B — 下降沿阶段触发
};
```

### 计数器与双 CMP 的时序关系

- 计数器从 0 上升到 ARR，再下降回 0，构成一个完整 PWM 周期。
- **CMP1**：CNT 上升过程中与比较值相等时触发，对应 PWM 输出的**前半周期事件**。
- **CMP3**：CNT 下降过程中与同一（或不同）比较值相等时触发，对应**后半周期事件**。
- 每周期共触发 **2 次** CMP 中断（上升 + 下降各一次）。

### 占空比与 CMP 值的关系

设 ARR = 8000（对应 100% 周期），占空比 $d$：

$$
\text{CMP1} = \text{CMP3} = \text{ARR} \times (1 - d)
$$

> 对称模式下 CMP1 == CMP3；非对称模式下两者不同，可产生相移或不对称波形。

### 双 CMP 中断的典型用途

| 中断点          | 触发时机         | 典型操作                          |
| --------------- | ---------------- | --------------------------------- |
| CMP1（上升沿）  | PWM 高电平开始前 | 更新下一周期占空比、触发 ADC 采样 |
| CMP3（下降沿）  | PWM 高电平结束后 | 读取 ADC 结果、执行控制算法       |
| OVF（ARR 顶点） | 计数器到达峰值   | 同步多路 PWM、周期性保护检测      |

### CMP 值更新时机

双 CMP 模式下，**必须在 OVF 或 CMP 中断内更新 CMP 寄存器**，避免在计数器运行中途写入导致毛刺：

```c
// 在 OVF 中断（计数器归零/到顶）时统一更新，保证下一周期生效
void PWM_OVF_IRQHandler(void)
{
    // 根据控制算法计算新占空比
    float dutyA = ControlLoop_GetDutyA();
    float dutyB = ControlLoop_GetDutyB();

    // 对称模式：CMP1 == CMP3
    uint16_t cmpA = (uint16_t)(ARR * (1.0f - dutyA));
    uint16_t cmpB = (uint16_t)(ARR * (1.0f - dutyB));

    PWM_SetCMP(CH_A, CMP1, cmpA);
    PWM_SetCMP(CH_A, CMP3, cmpA);
    PWM_SetCMP(CH_B, CMP1, cmpB);
    PWM_SetCMP(CH_B, CMP3, cmpB);
}
```

> 📎 异常进入/退出时 SP、LR、PC 寄存器的硬件行为（自动压栈、EXC_RETURN 等）详见 [PC/LR/SP 寄存器笔记](../PC_LR_SP寄存器.md)
