# PLL锁相环

[← 返回本模块 MOC](./MOC.md) | [← 返回 FOC MOC](../MOC.md) | [← 返回主页](../../../index.md)

---

## 一、核心概念：什么是PLL？

在无刷电机FOC（磁场定向控制）中，准确获取转子位置与速度是实现高性能控制的前提。锁相环（Phase-Locked Loop, PLL ）是一种经典的反馈控制系统，它能使其输出信号的相位与输入参考信号的相位同步。

在电机控制中，我们将观测器（如滑模观测器、龙伯格观测器）输出的反电动势（Back-EMF）或扩展反电动势信号中的位置误差信号作为PLL的输入，通过闭环调节， 精确地估算出转子的电角度和电角速度 。

PLL在FOC位置估算中的软件架构图

## 二、PLL在FOC中的工作原理

PLL的核心思想是将位置估算问题转化为相位跟踪问题。观测器输出的反电动势信号中包含了转子位置信息，但其直接计算得到的位置噪声大。PLL通过一个闭环系统平滑地跟踪这个相位。

### 工作流程：

1. 相位检测器（PD） ：计算观测器输出的反电动势矢量与PLL内部估算位置矢量之间的相位误差。 // 典型相位误差计算（基于反电动势） $\varepsilon = -E_\alpha \sin(\hat{\theta}) + E_\beta \cos(\hat{\theta})$ // 当$E_\alpha$, $E_\beta$为归一化的扩展反电动势时，$\varepsilon$正比于 $\sin(\theta - \hat{\theta})$
1. 环路滤波器（LF） ：通常是一个PI控制器，用于处理相位误差$\varepsilon$。PI输出即为估算的电角速度$\hat{\omega}$。其作用是滤除高频噪声，并提供环路稳定性。 // 离散PI控制器实现 $\hat{\omega} = K_p \varepsilon + K_i \sum \varepsilon T_s$; // $T_s$为控制周期
1. 压控振荡器（VCO） ：在数字PLL中，VCO通常是一个积分器。将估算的速度$\hat{\omega}$积分，得到估算的转子位置$\hat{\theta}$。 $\hat{\theta} = \hat{\theta} + \hat{\omega} T_s$; // 前向欧拉积分 // 注意处理角度溢出（模 $2\pi$） if(θ̂ > 2π) θ̂ = θ̂ - 2π; if(θ̂ < 0) θ̂ = θ̂ + 2π;
**讲师提示：**PLL本质上是一个二阶系统。PI控制器的参数$K_p$和$K_i$决定了环路的带宽、响应速度和抗噪性。带宽过高会引入噪声，过低则动态响应慢。
## 三、PLL设计关键参数与调参指南

PLL的性能主要由环路滤波器（PI）的参数决定。我们可以将其类比为一个二阶低通滤波器进行分析。

| 参数符号 | 物理意义 | 影响 | 设计公式（近似） |
| --- | --- | --- | --- |
| $\omega_n$ | 环路自然频率（带宽） | 决定系统响应速度。$\omega_n$ 越大，跟踪越快，但噪声抑制越差。 | 根据电机最大加速度和允许误差选择 |
| $\zeta$ | 阻尼比 | 决定系统稳定性与超调。通常设为0.7~1.0，获得较好动态性能。 | 推荐 $\zeta = 0.707$ (最佳阻尼) |
| $K_p$ | 比例增益 | 直接响应误差，提高动态性能。 | $K_p = 2 \zeta \omega_n$ |
| $K_i$ | 积分增益 | 消除稳态相位误差，提高低频跟踪精度。 | $K_i = \omega_n^2$ |

### 调参步骤：

- 步骤1 ：确定系统采样周期$T_s$ 。
- 步骤2 ：根据电机应用需求（如最大速度、加速度）和噪声水平，设定目标环路带宽$\omega_n$ 。通常$\omega_n$ 设置为电机最高电频率的2~5倍。
- 步骤3 ：设定阻尼比$\zeta = 0.7 \sim 1.0$。
- 步骤4 ：利用公式计算$K_p$ 和$K_i$ 的连续域值。
- 步骤5 ：使用离散化方法（如双线性变换）将PI控制器离散化，得到嵌入式代码可用的参数。

## 四、嵌入式C代码实现示例

以下是一个基于ARM Cortex-M内核的简化的PLL估算器C语言实现，适用于FOC算法中的位置速度估算环节。
```c
/**
 * @brief PLL估算器结构体
 */
typedef struct {
    float theta_hat; // 估算转子位置 (rad)
    float omega_hat; // 估算转子电速度 (rad/s)
    float epsilon;   // 相位误差
    float Kp;        // 比例增益
    float Ki;        // 积分增益
    float integral;  // 积分项累加值
    float Ts;        // 控制周期 (s)
    float max_omega; // 速度限幅 (rad/s)
} PLL_Estimator;

/**
 * @brief 初始化PLL估算器
 */
void PLL_Init(PLL_Estimator *pll, float Kp, float Ki, float Ts, float max_omega)
{
    pll->theta_hat = 0.0f;
    pll->omega_hat = 0.0f;
    pll->epsilon = 0.0f;
    pll->Kp = Kp;
    pll->Ki = Ki;
    pll->integral = 0.0f;
    pll->Ts = Ts;
    pll->max_omega = max_omega;
}

/**
 * @brief 执行一步PLL估算
 * @param Ealpha, Ebeta: 观测器输出的（扩展）反电动势分量
 */
void PLL_Update(PLL_Estimator *pll, float Ealpha, float Ebeta)
{
    // 1. 相位检测器 (PD): 计算相位误差
    // 假设 Ealpha, Ebeta 已包含 sin(θ), cos(θ) 信息
    pll->epsilon = -Ealpha * sinf(pll->theta_hat) + Ebeta * cosf(pll->theta_hat);

    // 2. 环路滤波器 (PI控制器): 更新估算速度
    pll->integral += pll->epsilon * pll->Ki * pll->Ts; // 积分项
    pll->omega_hat = (pll->epsilon * pll->Kp) + pll->integral; // PI输出

    // 速度限幅 (可选，增加稳定性)
    if (pll->omega_hat > pll->max_omega) pll->omega_hat = pll->max_omega;
    if (pll->omega_hat < -pll->max_omega) pll->omega_hat = -pll->max_omega;

    // 3. 压控振荡器 (VCO): 积分速度得到位置
    pll->theta_hat += pll->omega_hat * pll->Ts;

    // 角度归一化到 [0, 2π)
    if (pll->theta_hat >= TWOPI) pll->theta_hat -= TWOPI;
    if (pll->theta_hat < 0.0f) pll->theta_hat += TWOPI;
}
```
## 五、PLL的优势与局限性

适用场景建议： PLL非常适合中高速运行的无感FOC控制。在极低速或零速下，需要结合其他技术（如高频注入、I-F启动）来提供初始位置或低速牵引。

## 六、本章总结与思考题

总结： 锁相环（PLL）是将观测器输出的粗糙位置信息，通过相位跟踪原理， 平滑、精确地 估算出转子位置和速度的强大工具。其核心是相位检测、PI滤波和积分环节构成的闭环系统。

### 思考题：

1. 如果增大PLL的带宽（$\omega_n$ ），估算出的位置信号会变得更“平滑”还是更“毛刺”？为什么？
1. 在电机启动瞬间，反电动势几乎为0，此时PLL的输入$\varepsilon$无效。如何设计启动策略让PLL能成功“锁相”？
1. 除了本文提到的基于反电动势的相位误差计算，还有哪些信号可以作为PLL的输入？（提示：考虑磁链）

