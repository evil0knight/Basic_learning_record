# 双环FOC

[← 返回本模块 MOC](./MOC.md) | [← 返回 FOC MOC](../MOC.md) | [← 返回主页](../../../index.md)

---

## 一、双环控制系统核心思想

双环控制是一种经典的 级联控制 结构。其核心思想是：

- 速度环（外环） ：根据目标速度与反馈速度的误差，计算出维持该速度所需的 电流参考值（通常是q轴电流） 。它关注的是宏观的运动性能（稳、准、快）。
- 电流环（内环） ：接收速度环给出的电流指令，快速、准确地控制电机相电流，使其跟踪指令。它关注的是微观的电磁力生成。

外环输出是内环的输入，内环的快速响应为外环的稳定控制提供了基础。

图1：双环FOC控制系统架构图

## 二、速度环的设计与实现

速度环通常也采用PI控制器。其输入是速度误差，输出是q轴电流的参考值 $I_{q\_ref}$ 。

### 关键步骤：

1. 速度反馈获取 ：通过编码器、霍尔传感器等测量电机机械角度，并对其进行差分（或使用观测器）得到机械转速 $\omega_m$ 。通常需要低通滤波以抑制噪声。
1. 速度PI控制器 ：计算 $I_{q\_ref} = K_{p\_sp} e_\omega + K_{i\_sp} \int e_\omega \, dt$ 。注意输出限幅，以保护电机和驱动器。
1. 前馈补偿（可选） ：在目标速度变化时，加入前馈项（如基于负载惯量的转矩补偿）可以改善动态响应。

### 速度环PI参数整定要点

| 参数 | 影响 | 整定原则 |
| --- | --- | --- |
| 比例增益 $K_{p\_sp}$ | 影响系统响应速度。过大易超调振荡，过小则响应慢。 | 从小逐渐增大，至系统出现轻微振荡，然后回调至80%。 |
| 积分增益 $K_{i\_sp}$ | 消除稳态误差。过大会引起积分饱和与低速抖动。 | 在$K_p$调好后，从零逐渐增加，直至稳态误差在要求时间内消除。 |
| 输出限幅 $I_{q\_max}$ | 限制最大电流指令，保护系统。 | 根据电机和驱动器的最大允许电流设定（通常为额定电流的1.2-2倍）。 |

## 三、双环系统的软件流程与代码框架

在嵌入式系统中，双环控制通常在定时中断（如PWM载波中断）中执行，电流环频率（如20kHz）高于速度环频率（如1-5kHz）。
双环FOC核心控制函数（伪代码框架）
```c
// 全局变量定义
float target_speed_rpm = 0.0;   // 目标速度 (RPM)
float electrical_angle = 0.0;   // 电角度
float Id_ref = 0.0;             // d轴电流参考 (通常为0，用于弱磁控制)
float Iq_ref = 0.0;             // q轴电流参考 (速度环输出)
float Vd = 0.0, Vq = 0.0;       // 电压指令

// PID 结构体
typedef struct {
    float Kp;
    float Ki;
    float integral;
    float output_limit;
} PID_Controller;

PID_Controller speed_pid, current_q_pid, current_d_pid;

// 速度环控制函数 (在较低频率的中断中调用，如1kHz)
void SpeedLoop_Control(float speed_feedback_rpm) {
    // 1. 计算速度误差
    float speed_error = target_speed_rpm - speed_feedback_rpm;

    // 2. 速度PI控制器
    speed_pid.integral += speed_error * speed_pid.Ki * SPEED_LOOP_PERIOD;
    // 抗积分饱和处理
    if (speed_pid.integral > speed_pid.output_limit) speed_pid.integral = speed_pid.output_limit;
    if (speed_pid.integral < -speed_pid.output_limit) speed_pid.integral = -speed_pid.output_limit;

    Iq_ref = speed_error * speed_pid.Kp + speed_pid.integral;
    // 输出限幅
    if (Iq_ref > speed_pid.output_limit) Iq_ref = speed_pid.output_limit;
    if (Iq_ref < -speed_pid.output_limit) Iq_ref = -speed_pid.output_limit;
}

// 电流环及FOC变换函数 (在较高频率的PWM中断中调用，如20kHz)
void CurrentLoop_FOC(float Ia, float Ib, float angle_elec) {
    // 1. Clarke 变换
    float I_alpha = Ia;
    float I_beta = (Ia + 2.0 * Ib) * ONE_BY_SQRT3; // 常数可预先计算

    // 2. Park 变换
    float sin_theta = sin(angle_elec);
    float cos_theta = cos(angle_elec);
    float Id = I_alpha * cos_theta + I_beta * sin_theta;
    float Iq = -I_alpha * sin_theta + I_beta * cos_theta;

    // 3. 电流环 PI 控制 (d轴和q轴)
    float Id_error = Id_ref - Id;
    float Iq_error = Iq_ref - Iq; // <-- 这里 Iq_ref 来自速度环！

    current_d_pid.integral += Id_error * current_d_pid.Ki;
    current_q_pid.integral += Iq_error * current_q_pid.Ki;
    // ... (限幅处理)

    Vd = Id_error * current_d_pid.Kp + current_d_pid.integral;
    Vq = Iq_error * current_q_pid.Kp + current_q_pid.integral;

    // 4. 逆Park变换
    float V_alpha = Vd * cos_theta - Vq * sin_theta;
    float V_beta = Vd * sin_theta + Vq * cos_theta;

    // 5. SVPWM 或 SPWM 生成占空比
    // ... (调用SVPWM计算函数，更新PWM比较寄存器)
}
```

## 四、调试技巧与常见问题

- “先内后外”调试原则 ：务必先调好电流环，确保电流能快速、准确地跟踪阶跃指令后，再调试速度环。
- 速度反馈滤波 ：位置差分得到的速度噪声大，必须使用低通滤波。但滤波过重会导致相位滞后，影响稳定性。需折中考虑。
- 低速抖动问题 ：可能是积分饱和、速度测量噪声、或机械摩擦引起。可尝试：加入积分分离、改进速度观测器（如锁相环PLL）、加入非线性补偿。
- 动态响应不足 ：检查速度环PI限幅是否过小，电流环带宽是否足够高以快速响应速度环的电流指令。
