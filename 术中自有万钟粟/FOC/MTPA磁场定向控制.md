# MTPA磁场定向控制

[← 返回 FOC MOC](./MOC.md) | [← 返回主页](../../index.md)

---

## 引言：FOC的核心目标

磁场定向控制（Field-Oriented Control, FOC）的核心思想是将电机的定子电流解耦为产生磁场的 直轴电流（$i_d$） 和产生转矩的 交轴电流（$i_q$） 。通过独立控制这两个分量，我们可以像控制直流电机一样高效、精准地控制永磁同步电机（PMSM）或感应电机。本讲将深入探讨两种最核心的电流控制策略： $i_d=0$控制 与 最大转矩电流比控制（MTPA） 。

## 0 第一部分：id=0 控制原理

$i_d=0$控制 是最简单、应用最广泛的FOC控制策略。其核心思想是令直轴电流分量为零（$i_d=0$），将所有定子电流都用于产生转矩（即全部为$i_q$分量）。

### 1.1 基本原理与数学描述

对于表贴式永磁同步电机（SPMSM），其dq轴电感相等（$L_d = L_q$）。电磁转矩方程为：

$$T_e = \frac{3}{2} P \left[ \Psi_f i_q + (L_d - L_q) i_d i_q \right]$$

当 $L_d = L_q$ 时，方程简化为：

$$T_e = \frac{3}{2} P \Psi_f i_q$$

此时，转矩与交轴电流 $i_q$ 成严格的 正比关系 ，实现了完全的线性解耦控制。

### 1.2 实现框图与控制流程

### 1.3 特点与优缺点

| 优点 | 缺点/局限 |
| --- | --- |
| 控制简单，实现容易 | 未利用磁阻转矩，对于凸极电机（$L_d \ne L_q$）非最优 |
| 转矩与$i_q$线性关系，控制直观 | 高速弱磁区域需要切换策略 |
| 对SPMSM效率较高 | 电流利用率非最大，相同转矩下定子电流较大 |
| 系统稳定，可靠性高 | 不适用于追求极致效率或体积受限的场合 |
**适用场景：**表贴式永磁同步电机（SPMSM）、对控制复杂度敏感、中低速运行、成本优先的应用。
## 第二部分：最大转矩电流比控制（MTPA）原理

最大转矩电流比控制（Maximum Torque Per Ampere, MTPA） 是一种优化控制策略，其目标是在给定转矩下，使定子电流幅值最小，从而降低铜耗、提高效率、减小发热。这对于内置式永磁同步电机（IPMSM，$L_d < L_q$）尤为重要。

### 2.1 数学推导与原理

对于凸极电机（$L_d \ne L_q$），转矩包含永磁转矩和磁阻转矩两部分。电流矢量 ($i_d$, $i_q$) 的幅值 $I_s$ 满足：

$$I_s^2 = i_d^2 + i_q^2$$

MTPA的目标是： 在满足转矩方程 $T_e(i_d, i_q) = T_{\text{ref}}$ 的约束下，最小化 $I_s$。这是一个条件极值问题，通过构造拉格朗日函数求解，得到MTPA轨迹条件：

$$i_d = \frac{\Psi_f}{L_q - L_d} - \sqrt{\left( \frac{\Psi_f}{L_q - L_d} \right)^2 + i_q^2}$$

或表示为 $i_d$ 与 $i_q$ 的关系：

$$i_q^2 = \frac{\Psi_f}{L_q - L_d} i_d - i_d^2$$

### 2.2 MTPA轨迹可视化

上图中， 红色MTPA轨迹 是每个等转矩曲线与最小等电流圆的切点连线。在轨迹上运行，可以用最小的电流产生所需的转矩。

### 2.3 实现方法

MTPA的实现通常有以下几种方式：

1. 公式计算法： 在线或离线根据上述公式，由转矩指令$T_{\text{ref}}$计算出最优的$i_{d,\text{ref}}$和$i_{q,\text{ref}}$。
1. 查表法： 离线计算MTPA轨迹点，制成表格存储在控制器中，运行时查表并插值。
1. 搜索法： 在线小幅扰动$i_d$，观察转矩变化，向转矩/电流比增大的方向调整$i_d$设定值。

```c
// 示例：MTPA查表法核心代码片段 (C语言)
typedef struct {
    float torque_ref; // 转矩指令 N.m
    float id_ref;     // 最优直轴电流参考值 A
    float iq_ref;     // 最优交轴电流参考值 A
} MTPA_LUT_Entry;

// MTPA 查表（简化示例）
void MTPA_TableLookup(float torque_cmd, float *id_ref, float *iq_ref) {
    static const MTPA_LUT_Entry mtpa_table[] = {
        {0.0,  0.00, 0.00},
        {0.5, -0.12, 1.85},
        {1.0, -0.25, 3.70},
        {1.5, -0.40, 5.52},
        {2.0, -0.55, 7.32},
        // ... 更多数据点
    };
    int table_size = sizeof(mtpa_table)/sizeof(mtpa_table[0]);
    // 查表与线性插值算法
    for(int i = 0; i < table_size-1; i++) {
        if(torque_cmd >= mtpa_table[i].torque_ref &&
           torque_cmd <= mtpa_table[i+1].torque_ref) {
            float ratio = (torque_cmd - mtpa_table[i].torque_ref) /
                          (mtpa_table[i+1].torque_ref - mtpa_table[i].torque_ref);
            *id_ref = mtpa_table[i].id_ref + ratio * (mtpa_table[i+1].id_ref - mtpa_table[i].id_ref);
            *iq_ref = mtpa_table[i].iq_ref + ratio * (mtpa_table[i+1].iq_ref - mtpa_table[i].iq_ref);
            return;
        }
    }
    // 超范围处理
    *id_ref = mtpa_table[table_size-1].id_ref;
    *iq_ref = mtpa_table[table_size-1].iq_ref;
}
```

## 第三部分：id=0 与 MTPA 对比与应用选择

| 对比维度 | $i_d=0$ 控制 | MTPA 控制 |
| --- | --- | --- |
| 核心思想 | 令直轴电流为零，全部电流用于产生转矩 | 优化$i_d$和$i_q$分配，使单位电流产生转矩最大 |
| 数学复杂度 | 简单，线性关系 | 复杂，涉及非线性方程或查表 |
| 计算资源 | 需求低 | 需求较高（计算或存储） |
| 电机类型 | 最适合SPMSM ($L_d = L_q$) | 最适合IPMSM ($L_d < L_q$)，优势明显 |
| 效率与发热 | 一般，铜耗相对较大 | 优，相同转矩下电流最小，铜耗低 |
| 控制性能 | 动态响应好，稳定 | 动态响应可能略慢，但稳态效率高 |
| 典型应用 | 风扇、泵、低成本驱动器、SPMSM | 电动汽车、伺服系统、机器人、IPMSM |

### 3.1 如何选择？

- 选择 $i_d=0$ 控制 if： 电机是表贴式（SPMSM）；项目对成本、开发周期敏感；运行速度主要在基速以下；对效率要求不是极端苛刻。
- 选择 MTPA 控制 if： 电机是内置式（IPMSM）；追求系统最高效率（如电动汽车）；需要最大化利用电池能量；散热条件有限，需要最小化发热。
**进阶提示：**在实际高性能驱动器中，通常采用**混合策略**：低速区采用MTPA，高速区切换到弱磁控制（Flux Weakening），以实现全速度范围的最优性能。
