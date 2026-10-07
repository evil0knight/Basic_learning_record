# SVPWM原理

[← 返回本模块 MOC](./MOC.md) | [← 返回 FOC MOC](../MOC.md) | [← 返回主页](../../../index.md)

---

### 一、电压空间矢量 (Voltage Space Vector) 概念

在电机控制中，我们通常关注的是施加在电机三相绕组（A, B, C）上的电压。FOC的核心思想之一，就是将三相静止坐标系（ABC）下的变量（电压、电流、磁链）转换到两相旋转坐标系（dq）下来进行控制。

电压空间矢量 是一个用来 综合表示三相电压瞬时状态 的合成矢量。它位于一个复平面（α-β平面）上，其方向和长度代表了电机内部合成磁势的方向和强度。

#### 核心公式： Clarke变换 (3相 -> 2相静止)

将三相电压 $U_A, U_B, U_C$ 转换为两相静止坐标系（α-β）下的分量 $U_\alpha, U_\beta$：

```c
// Clarke 变换 C代码示例
void Clarke_Transform(float uA, float uB, float uC, float *uAlpha, float *uBeta) {
    // 假设三相电压和为0 (uA + uB + uC = 0)
    *uAlpha = uA; // 或使用等幅值变换： *uAlpha = (2/3)*(uA - 0.5*uB - 0.5*uC);
    *uBeta = (uA + 2*uB) / sqrt(3); // 等幅值变换： *uBeta = (sqrt(3)/3)*(uB - uC);
}
```

则电压空间矢量 $U_s$ 可表示为： $U_s = U_\alpha + jU_\beta$。

### 二、基本电压矢量与逆变器开关状态

我们常用的三相两电平电压源逆变器有6个开关管（通常为IGBT或MOSFET），其拓扑结构如下图所示：

每个桥臂的上下开关管互补导通，防止直通。因此，我们可以用三个二进制位 (Sa, Sb, Sc) 来表示逆变器的开关状态，其中 1 表示上管导通、下管关断， 0 表示下管导通、上管关断。

三相共有 $2^3 = 8$ 种开关组合，对应8个 基本电压空间矢量 ：6个 非零矢量 （U1 ~ U6）和2个 零矢量 （U0, U7）。

| 开关状态 (Sa Sb Sc) | 矢量符号 | 矢量名称 | α-β 坐标 (Uα, Uβ) | 相位角 |
| --- | --- | --- | --- | --- |
| 0 0 0 | U0 | 零矢量 | (0, 0) | - |
| 1 0 0 | U1 | 非零矢量 | (2/3 Vdc, 0) | 0° |
| 1 1 0 | U2 | 非零矢量 | (1/3 Vdc, √3/3 Vdc) | 60° |
| 0 1 0 | U3 | 非零矢量 | (-1/3 Vdc, √3/3 Vdc) | 120° |
| 0 1 1 | U4 | 非零矢量 | (-2/3 Vdc, 0) | 180° |
| 0 0 1 | U5 | 非零矢量 | (-1/3 Vdc, -√3/3 Vdc) | 240° |
| 1 0 1 | U6 | 非零矢量 | (1/3 Vdc, -√3/3 Vdc) | 300° |
| 1 1 1 | U7 | 零矢量 | (0, 0) | - |

### 三、扇区 (Sector) 划分

为了用这8个离散的基本矢量来合成任意方向和大小的目标电压矢量 $U_{ref}$，我们将α-β平面平均划分为 6个扇区 （每个扇区60°）。

判断目标矢量 $U_{ref} (U_\alpha, U_\beta)$ 所在扇区是SVPWM算法的第一步。常用的判断方法是基于 $U_\alpha, U_\beta$ 计算三个中间变量：

```c
// 扇区判断 C代码示例
int Sector_Judgment(float Ualpha, float Ubeta) {
    int sector = 0;
    float A, B, C;
    // 计算三个参考量
    A = Ubeta;
    B = (sqrt(3)*Ualpha - Ubeta) / 2;
    C = (-sqrt(3)*Ualpha - Ubeta) / 2;

    // 判断逻辑
    if(A > 0) sector += 1;
    if(B > 0) sector += 2;
    if(C > 0) sector += 4;

    // 映射到标准扇区编号1~6
    switch(sector) {
        case 3: return 1;
        case 1: return 2;
        case 5: return 3;
        case 4: return 4;
        case 6: return 5;
        case 2: return 6;
        default: return 0; // 理论上不会出现，除非矢量为0
    }
}
```

#### 扇区判断规则表（基于符号判断法）

定义： $V_{ref1} = U_\beta$, $V_{ref2} = \frac{\sqrt{3}}{2}U_\alpha - \frac{1}{2}U_\beta$, $V_{ref3} = -\frac{\sqrt{3}}{2}U_\alpha - \frac{1}{2}U_\beta$

令：若 $V_{ref1} > 0$，则 N=1，否则 N=0；若 $V_{ref2} > 0$，则 N=2，否则 N=0；若 $V_{ref3} > 0$，则 N=4，否则 N=0。

扇区号 = N 的和，然后按下表映射：

| N值 | 1 | 2 | 3 | 4 | 5 | 6 |
| --- | --- | --- | --- | --- | --- | --- |
| 扇区号 | II | VI | I | IV | III | V |

### 四、SVPWM在FOC中的位置

#### FOC控制环路软件架构图

如图所示，SVPWM模块接收来自反Park变换输出的目标电压矢量 $U_\alpha, U_\beta$，通过 扇区判断、矢量作用时间计算、PWM波形生成 三个核心步骤，最终生成驱动逆变器六个开关管的PWM信号，从而在电机端合成出所需的目标电压矢量。
