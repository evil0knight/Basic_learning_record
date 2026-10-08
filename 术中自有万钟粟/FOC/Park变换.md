# Park变换

[← 返回 FOC MOC](./MOC.md) | [← 返回主页](../../index.md)

---

## 一、Park变换的核心思想

在上一章的Clarke变换中，我们将三相静止坐标系（ABC）下的变量转换到了两相静止坐标系（α-β）。然而，电机转子是旋转的，其磁场在空间中以电角速度 $\omega$ 旋转。为了实现对转子磁场的直接、解耦控制，我们需要将变量转换到一个与转子磁场同步旋转的坐标系中，这就是 两相旋转坐标系（d-q坐标系） 。

Park变换（又称2s/2r变换）的物理意义在于：将静止的观察视角，转变为骑在转子磁场上、随其一同旋转的观察视角。在这个旋转的视角下，原本在静止坐标系中呈正弦变化的交流量（如电流、电压），就变成了相对恒定的直流量，从而极大简化了控制器的设计。

核心目标：通过一个旋转变换，将静止坐标系（α-β）中的矢量投影到与转子磁场同步旋转的直轴（d轴）和交轴（q轴）上，从而实现对转矩分量（q轴电流）和磁场分量（d轴电流）的独立控制。

## 二、数学推导

### 2.1 几何关系推导

设静止坐标系（α-β）中有一个空间矢量 $I_s = [I_\alpha, I_\beta]^T$。旋转坐标系（d-q）以电角速度 $\omega_e$ 逆时针旋转，其d轴与α轴的瞬时夹角为电角度 $\theta_e = \omega_e t$。

将矢量 $I_s$ 分别向d轴和q轴投影，利用三角函数关系，可以得到其在旋转坐标系下的分量：

$$\begin{aligned}
I_d &= I_\alpha \cos\theta_e + I_\beta \sin\theta_e \\
I_q &= -I_\alpha \sin\theta_e + I_\beta \cos\theta_e
\end{aligned}$$

### 2.2 矩阵形式

将上述标量方程写成紧凑的矩阵形式，即得到Park变换的正变换公式：
**$I_{dq} = P(\theta) \cdot I_{\alpha\beta}$**
| [ | I d | ] | = | [ | cosθ e | sinθ e | ] | · | [ | I α | ] |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| [ | I q | ] | -sinθ e | cosθ e | I β |  |  |  |  |  |  |

### 2.3 逆变换

逆 Park 变换已拆分为独立模块，详见[逆 Park 变换](逆Park变换.md)。完整的正变换、逆变换代码和调用流程见[坐标变换代码实现](坐标变换代码实现.md)。
### 💡 关键理解点

- d轴（直轴） ：与转子永磁体磁场方向对齐，控制d轴电流（$I_d$）可以增强或削弱气隙磁场（用于弱磁控制）。
- q轴（交轴） ：超前d轴90度电角度，控制q轴电流（$I_q$）直接产生电磁转矩。在永磁同步电机（PMSM）中，转矩与 $I_q$ 成正比。
- 变换的核心 ：变换矩阵 $P(\theta)$ 中的角度 $\theta_e$ 必须是转子的实时电角度，通常由位置传感器（如编码器）或观测器（如滑模观测器）提供。

## 三、软件实现（C语言）

在嵌入式系统中，Park变换的实现需要高效且避免浮点运算（如果主控无FPU）。通常采用定点数运算或查表法（LUT）计算三角函数。

### 3.1 数据结构定义

```c
/* FOC常用数据结构 */
typedef struct {
    float alpha;   // α轴分量
    float beta;    // β轴分量
} AlphaBeta_t;

typedef struct {
    float d;       // d轴分量（直轴，磁场分量）
    float q;       // q轴分量（交轴，转矩分量）
} DQ_t;

typedef struct {
    float sin_val;
    float cos_val;
} Trig_Components_t;
```

### 3.2 Park变换函数实现

```c
/**
  * @brief  Park正变换：从静止坐标系(α,β)变换到旋转坐标系(d,q)
  * @param  pInput: 指向输入结构体（α-β分量）的指针
  * @param  pOutput: 指向输出结构体（d-q分量）的指针
  * @param  pTrig: 指向包含sinθ和cosθ的结构体指针
  * @retval 无
  */
void Park_Transform(AlphaBeta_t *pInput, DQ_t *pOutput, Trig_Components_t *pTrig)
{
    /* I_d = I_α * cosθ + I_β * sinθ */
    pOutput->d = pInput->alpha * pTrig->cos_val + pInput->beta * pTrig->sin_val;

    /* I_q = -I_α * sinθ + I_β * cosθ */
    pOutput->q = -pInput->alpha * pTrig->sin_val + pInput->beta * pTrig->cos_val;
}
```

### 3.3 代码实现

本页保留 Park 正变换实现；逆 Park 实现统一见[逆 Park 变换](逆Park变换.md)，完整坐标变换流程见[坐标变换代码实现](坐标变换代码实现.md)。
### 3.4 三角函数计算优化

在实时控制中，频繁调用标准库的sin/cos函数开销巨大。常用优化方法：

| 方法 | 原理 | 精度 | 速度 | 适用场景 |
| --- | --- | --- | --- | --- |
| 查表法(LUT) | 预计算sin/cos值存入数组，根据角度索引查表 | 中高（取决于表大小） | 极快 | 资源有限的MCU，固定频率控制 |
| 泰勒展开 | 在零点附近用多项式逼近 | 中（取决于阶数） | 较快 | 有FPU的MCU，角度范围小 |
| CORDIC算法 | 通过迭代移位和加减计算 | 可配置 | 中（迭代次数决定） | 无硬件乘除器的MCU，ASIC/FPGA |
| 硬件FPU | 直接调用数学库 | 高 | 快（依赖硬件） | 高性能MCU（如Cortex-M4/M7） |

```c
/* 示例：查表法获取三角函数值（假设角度已归一化到0-4095对应0-2π） */
#define TRIG_TABLE_SIZE 4096
float sin_table[TRIG_TABLE_SIZE];
float cos_table[TRIG_TABLE_SIZE];

void Init_TrigTable(void) {
    for(int i=0; i<TRIG_TABLE_SIZE; i++) {
        float angle = 2.0f * PI * i / TRIG_TABLE_SIZE;
        sin_table[i] = sinf(angle);
        cos_table[i] = cosf(angle);
    }
}

Trig_Components_t Get_TrigValues(uint16_t angle_idx) {
    Trig_Components_t trig;
    trig.sin_val = sin_table[angle_idx];
    trig.cos_val = cos_table[angle_idx];
    return trig;
}
```

## 四、在FOC控制环中的位置

Park变换是FOC控制算法承上启下的核心环节。下图展示了其在典型FOC控制架构中的位置：
