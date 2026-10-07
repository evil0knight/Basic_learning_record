# 单环FOC

[← 返回本模块 MOC](./MOC.md) | [← 返回 FOC MOC](../MOC.md) | [← 返回主页](../../../index.md)

---

## 一、课程目标

本讲将带领大家构建一个完整的 单电流环磁场定向控制（FOC）系统 。我们将把之前学到的核心模块——坐标变换（Clark/Park）、空间矢量脉宽调制（SVPWM）、电流采样与PI控制器——有机地整合在一起，形成一个能够对电机相电流进行精确闭环控制的完整算法框架。这是实现高性能无刷电机控制的基石。

## 二、系统架构与数据流

单电流环FOC，也称为 电流转矩控制 ，其核心思想是控制定子电流矢量，使其与转子磁场垂直（q轴），从而产生最大转矩。系统闭环运行，实时采样电流，与给定值比较，通过PI调节器输出控制量。

### 单电流环FOC软件架构图

图1：单电流环FOC控制算法数据流图

#### 🔑 核心闭环流程：

1. 采样与变换 ：通过ADC采样两相电流（$I_a$, $I_b$），经Clark变换得到静止坐标系分量($I_\alpha$, $I_\beta$)，再结合转子电角度$\theta$，经Park变换得到旋转坐标系下的直轴电流$I_d$和交轴电流$I_q$。
1. 误差计算与调节 ：将$I_d$、$I_q$与它们的目标值$I_{d\_ref}$、$I_{q\_ref}$进行比较，产生误差信号。通常，对于永磁同步电机（PMSM），我们令$I_{d\_ref} = 0$（最大转矩控制），$I_{q\_ref}$由速度环或转矩指令给出。误差信号送入两个独立的PI控制器（分别对应d轴和q轴）。
1. 反变换与调制 ：PI控制器输出旋转坐标系下的电压指令$V_d$和$V_q$。通过逆Park变换，将其转换回静止坐标系下的电压指令$V_\alpha$和$V_\beta$。最后，将$V_\alpha$、$V_\beta$送入SVPWM模块，生成驱动三相逆变器的六路PWM信号。
1. 执行与反馈 ：PWM信号控制功率MOSFET/IGBT，产生施加在电机绕组上的三相电压，从而产生期望的电流。电机转动，位置传感器（如编码器）反馈新的角度$\theta$，开启下一个控制周期。

## 三、关键模块整合要点

| 模块 | 输入 | 输出 | 整合注意事项 |
| --- | --- | --- | --- |
| 电流采样与处理 | ADC原始值 ($I_{a\_raw}$, $I_{b\_raw}$) | 标幺化电流值 ($I_a$, $I_b$) | 需校准ADC偏移、增益。采样时刻需与PWM中心对齐，以避开开关噪声。通常采用双电阻或单电阻采样法。 |
| 坐标变换链 | $I_a$, $I_b$, $\theta$ | $I_d$, $I_q$ | 确保角度$\theta$的准确性和实时性。Park/逆Park变换使用相同的$\theta$。注意三角函数计算的效率（可查表或使用CORDIC算法）。 |
| PI控制器 | $I_{d\_err}$, $I_{q\_err}$ | $V_d$, $V_q$ | 需进行抗积分饱和处理。输出限幅至关重要，限幅值应与直流母线电压和SVPWM线性区匹配。通常先进行离散化（如位置式或增量式PI）。 |
| SVPWM | $V_\alpha$, $V_\beta$, $V_{dc}$ | PWM1-6占空比 | 输入电压需进行标幺化（除以$V_{dc}/\sqrt{3}$）。计算出的占空比要匹配定时器的计数范围。注意插入死区时间以防止上下管直通。 |

## 四、核心代码框架示例 (C语言)

以下是一个简化的、面向嵌入式单片机的单电流环FOC主中断服务程序（ISR）框架，通常在PWM定时器下溢或周期中断中执行。

```c
/**
  * @brief FOC单电流环计算中断服务函数
  * @note 假设在PWM中心对齐模式的下溢中断中调用，频率为20kHz
  */
void FOC_CurrentLoop_ISR(void) {
    // --- 1. 读取反馈 ---
    // 1.1 读取ADC值并转换为实际电流 (假设已校准)
    gFOC.Ia = ADC_GetCurrentA(); // 单位：A
    gFOC.Ib = ADC_GetCurrentB();
    // 1.2 读取转子电角度 (0 ~ 2π)
    gFOC.theta = ENC_GetElectricalAngle();

    // --- 2. 坐标变换 (ABC -> dq) ---
    // Clark变换: (Ia, Ib) -> (Iα, Iβ)
    gFOC.Ialpha = gFOC.Ia;
    gFOC.Ibeta  = (gFOC.Ia + 2.0f * gFOC.Ib) * ONE_BY_SQRT3; // 常数已定义

    // Park变换: (Iα, Iβ) -> (Id, Iq)
    float sin_theta, cos_theta;
    fast_sin_cos(gFOC.theta, &sin_theta, &cos_theta); // 快速三角函数计算
    gFOC.Id =  cos_theta * gFOC.Ialpha + sin_theta * gFOC.Ibeta;
    gFOC.Iq = -sin_theta * gFOC.Ialpha + cos_theta * gFOC.Ibeta;

    // --- 3. PI 控制器 (电流环) ---
    // 3.1 计算误差 (目标值通常来自上层，这里Id_ref=0, Iq_ref由外部给定)
    float Id_err = gFOC.Id_ref - gFOC.Id;
    float Iq_err = gFOC.Iq_ref - gFOC.Iq;

    // 3.2 执行PI运算 (以增量式PI为例，避免积分饱和)
    gFOC.Vd = PI_Controller(&gFOC.pi_d, Id_err); // PI_Controller函数内部实现增量式PI及限幅
    gFOC.Vq = PI_Controller(&gFOC.pi_q, Iq_err);

    // --- 4. 反Park变换 (dq -> αβ) ---
    gFOC.Valpha = cos_theta * gFOC.Vd - sin_theta * gFOC.Vq;
    gFOC.Vbeta  = sin_theta * gFOC.Vd + cos_theta * gFOC.Vq;

    // --- 5. SVPWM 调制 ---
    // 将电压矢量 (Valpha, Vbeta) 转换为三相占空比
    SVPWM_CalcDutyCycle(gFOC.Valpha, gFOC.Vbeta, gFOC.Vdc, gFOC.duty);

    // --- 6. 更新PWM比较寄存器 ---
    PWM_SetDutyCycle(gFOC.duty);
}
```

关键数据结构定义示例：

```c
typedef struct {
    // 电流反馈
    float Ia, Ib;
    float Ialpha, Ibeta;
    float Id, Iq;
    // 角度
    float theta;
    // 目标值
    float Id_ref; // 通常设为0
    float Iq_ref; // 转矩指令
    // PI控制器输出 (电压指令)
    float Vd, Vq;
    float Valpha, Vbeta;
    // 系统参数
    float Vdc; // 直流母线电压
    // PI控制器结构体
    PI_Regulator_t pi_d, pi_q;
    // 输出
    float duty[3]; // 三相占空比
} FOC_HandleTypeDef;

extern FOC_HandleTypeDef gFOC;
```

## 五、调试与性能优化建议

- 开环验证 ：先让系统在开环下运行（固定角度递增，给定$V_d$/$V_q$），用示波器观察相电流波形是否为正弦波，验证坐标变换和SVPWM的正确性。
- PI参数整定 ： 先将积分系数$K_i$设为0，逐步增大比例系数$K_p$，直到系统出现等幅振荡。 记录此时的振荡周期$T$和$K_p$值（临界增益$K_c$）。 根据齐格勒-尼科尔斯法则或其他经验公式设置$K_p$和$K_i$。 先调q轴环（转矩环），再调d轴环。
- 抗干扰措施 ： ADC采样软件滤波（如滑动平均）。 在PI控制器中加入输出限幅和积分抗饱和。 确保电源稳定，功率地信号地分离。
- 效率优化 ：使用查表法或CORDIC算法加速三角函数计算；将频繁调用的函数声明为内联（inline）；合理使用定点数运算（Q格式）以在无FPU的MCU上提升速度。
