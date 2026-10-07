# 逆Park变换

[← 返回本模块 MOC](./MOC.md) | [← 返回 FOC MOC](../MOC.md) | [← 返回主页](../../../index.md)

---

## 一、本章学习目标

- 理解反Park变换在FOC控制环中的位置与作用。
- 掌握从旋转坐标系（d-q）到静止坐标系（α-β）的数学推导过程。
- 学会在嵌入式C代码中实现高效、准确的反Park变换。
- 能够分析变换过程中的关键参数与误差来源。

## 二、反Park变换在FOC中的位置

在磁场定向控制（FOC）中，反Park变换是电流环输出的关键一步。它将控制器计算出的、在同步旋转坐标系（d-q轴）上的电压指令 $V_d$ 和 $V_q$，转换回两相静止坐标系（α-β轴），以便进行后续的SVPWM调制。

### FOC控制算法软件架构图

反Park变换是连接“旋转世界”（控制器输出）和“静止世界”（逆变器输入）的桥梁。

## 三、数学推导：从旋转到静止

反Park变换是Park变换的逆过程。Park变换利用转子电角度 $\theta$ 将静止坐标系的量投影到旋转坐标系上。反Park变换则利用相同的角度 $\theta$，将旋转坐标系的量“反投影”回静止坐标系。

### 推导步骤

1. 已知量 ：旋转坐标系下的电压分量 $V_d$, $V_q$，以及当前的电角度 $\theta$（来自位置传感器或观测器）。
1. 几何关系 ：将 $V_d$ 和 $V_q$ 视为一个矢量 $V$ 在d-q坐标系上的两个分量。我们需要找到同一个矢量 $V$ 在α-β坐标系上的分量 $V_\alpha$ 和 $V_\beta$。
1. 坐标旋转 ：这相当于将d-q坐标系（它本身相对于α-β系旋转了 $\theta$ 角）中的坐标，通过一个反向旋转（$-\theta$ 角）变换回α-β系。

$$\begin{aligned}
V_\alpha &= V_d \cos\theta - V_q \sin\theta \\
V_\beta &= V_d \sin\theta + V_q \cos\theta
\end{aligned}$$

该变换矩阵形式为：

$$\begin{bmatrix} V_\alpha \\ V_\beta \end{bmatrix} = \begin{bmatrix} \cos\theta & -\sin\theta \\ \sin\theta & \cos\theta \end{bmatrix} \begin{bmatrix} V_d \\ V_q \end{bmatrix}$$

核心思想：将d轴和q轴上的分量，分别向α轴和β轴进行投影并求和，即可得到静止坐标系下的分量。

## 四、关键特性与物理意义

| 特性 | 说明 | 物理/工程意义 |
| --- | --- | --- |
| 线性变换 | 变换由线性矩阵描述，满足叠加原理。 | 控制器输出的 $V_d$ 和 $V_q$ 可以独立进行变换后再合成，便于分析和实现。 |
| 正交保范 | 变换是正交的，矢量模长不变：$V_\alpha^2 + V_\beta^2 = V_d^2 + V_q^2$。 | 确保电压矢量的幅值在变换前后保持不变，能量守恒。 |
| 角度依赖 | 变换矩阵元素是角度 $\theta$ 的三角函数。 | 变换的正确性高度依赖于转子位置角 $\theta$ 的实时性和准确性。 |
| 计算量小 | 仅需4次乘法、2次加减法（每个输出）。 | 非常适合在嵌入式MCU中每个PWM周期实时执行。 |

## 五、C语言实现与优化

在嵌入式系统中，需要高效、准确地实现上述公式。关键点在于三角函数值的获取和定点数运算。

### 5.1 基础浮点实现

```c
/**
 * @brief 反Park变换 (浮点版本)
 * @param Vd 旋转坐标系d轴电压分量
 * @param Vq 旋转坐标系q轴电压分量
 * @param sin_theta 当前电角度正弦值 sin(theta)
 * @param cos_theta 当前电角度余弦值 cos(theta)
 * @param[out] Valpha 静止坐标系α轴电压分量
 * @param[out] Vbeta  静止坐标系β轴电压分量
 */
void InversePark_Transform_Float(float Vd, float Vq,
                                 float sin_theta, float cos_theta,
                                 float *Valpha, float *Vbeta)
{
    *Valpha = Vd * cos_theta - Vq * sin_theta;
    *Vbeta  = Vd * sin_theta + Vq * cos_theta;
}
```

### 5.2 定点数优化实现（Q格式）

在实际电机控制芯片（如Cortex-M3/M4）中，常使用定点数运算以提高速度。假设使用Q15格式（1位符号位，15位小数位）。

```c
#include <stdint.h>

#define Q15_SHIFT 15
#define Q15_MULT(a, b) ((int32_t)(a) * (int32_t)(b) >> Q15_SHIFT) // 简化宏，注意溢出

/**
 * @brief 反Park变换 (Q15定点版本)
 * @param Vd_q15 Q15格式的Vd
 * @param Vq_q15 Q15格式的Vq
 * @param sin_theta_q15 Q15格式的sin(theta)
 * @param cos_theta_q15 Q15格式的cos(theta)
 * @param[out] Valpha_q15 Q15格式的Valpha
 * @param[out] Vbeta_q15  Q15格式的Vbeta
 */
void InversePark_Transform_Q15(int16_t Vd_q15, int16_t Vq_q15,
                               int16_t sin_theta_q15, int16_t cos_theta_q15,
                               int16_t *Valpha_q15, int16_t *Vbeta_q15)
{
    int32_t temp1, temp2;

    // Valpha = Vd * cosθ - Vq * sinθ
    temp1 = (int32_t)Vd_q15 * cos_theta_q15;
    temp2 = (int32_t)Vq_q15 * sin_theta_q15;
    *Valpha_q15 = (int16_t)((temp1 - temp2) >> Q15_SHIFT);

    // Vbeta = Vd * sinθ + Vq * cosθ
    temp1 = (int32_t)Vd_q15 * sin_theta_q15;
    temp2 = (int32_t)Vq_q15 * cos_theta_q15;
    *Vbeta_q15 = (int16_t)((temp1 + temp2) >> Q15_SHIFT);
}
```

### 实现要点提示

- 三角函数表 ：对于资源受限的MCU，常使用预先计算好的sin/cos查找表（LUT）来避免实时计算，通过角度索引直接取值。
- 角度归一化 ：提供给变换的角度 $\theta$ 通常是电角度，需要归一化到 $[0, 2\pi)$ 或对应查找表范围。
- 溢出保护 ：定点数乘法运算存在溢出风险，使用int32_t中间变量进行运算是标准做法。
- 执行时机 ：反Park变换在每个PWM控制周期（通常10-100kHz）执行一次，必须保证其计算耗时远小于PWM周期。

## 六、常见问题与调试技巧

| 问题现象 | 可能原因 | 排查与解决方法 |
| --- | --- | --- |
| 电机振动、噪音大 | 反Park变换输出的 $V_\alpha$、$V_\beta$ 波形畸变。 | 1. 检查输入角度 $\theta$ 是否正确、连续。 2. 检查sin/cos值（查表或计算）是否正确。 3. 用示波器或仿真观察 $V_\alpha$、$V_\beta$ 波形是否为正弦。 |
| 电机转矩偏小或效率低 | 变换过程中存在幅值衰减。 | 1. 验证变换的正交性：计算 $V_\alpha^2 + V_\beta^2$ 与 $V_d^2 + V_q^2$ 是否相等。 2. 检查定点数运算的精度损失，适当提高Q格式位数。 |
| 电机无法启动或失控 | 变换符号错误或角度相位错误。 | 1. 核对变换公式符号是否与Park变换对应。 2. 确认角度 $\theta$ 的零点定义与Park变换、编码器安装是否一致。 |
| CPU负载过高 | 三角函数计算耗时过长。 | 1. 换用查找表法。 2. 使用芯片自带的三角函数计算单元（如Cortex-M4的FPU）。 3. 优化代码，使用汇编或编译器优化选项。 |
