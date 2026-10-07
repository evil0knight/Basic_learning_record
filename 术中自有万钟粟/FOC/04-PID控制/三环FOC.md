# 三环FOC

[← 返回本模块 MOC](./MOC.md) | [← 返回 FOC MOC](../MOC.md) | [← 返回主页](../../../index.md)

---

## 本讲核心目标

在前述电流环、速度环双闭环FOC控制的基础上，引入 位置环 ，构建完整的 三环伺服控制系统 ，实现对无刷电机 角度、位置的精确、快速、稳定控制 。这是实现机器人关节、云台、精密转台等应用的关键。

## 三环伺服FOC系统架构图

## 位置环的核心作用与设计要点

### 核心作用

- 最外环控制 ：接收位置指令，输出速度指令给速度环。
- 消除静差 ：通过积分项累积位置误差，实现零稳态误差。
- 轨迹跟踪 ：控制电机按预定角度轨迹运动（点到点、匀速、S曲线）。
- 抗干扰与刚度 ：提供位置刚度，抵抗负载扰动，保持位置锁定。

### 设计要点

- PID调节 ：通常使用PID控制器，P提供刚度，I消除静差，D抑制超调。
- 前馈补偿 ：加入速度前馈、加速度前馈提升动态响应。
- 抗饱和处理 ：对积分项和输出进行限幅，防止Windup。
- 分辨率匹配 ：编码器分辨率与控制精度要求匹配。

## 三环PID参数整定顺序与特性

| 控制环 | 整定顺序 | 主要作用 | 典型带宽 | 关键参数影响 |
| --- | --- | --- | --- | --- |
| 电流环 (内环) | 1 | 控制转矩，快速响应电流指令 | 500 Hz - 2 kHz | 响应最快，决定系统动态基础 |
| 速度环 (中环) | 2 | 调节转速，平滑速度变化 | 50 Hz - 200 Hz | 带宽低于电流环5-10倍 |
| 位置环 (外环) | 3 | 精确定位，跟踪位置轨迹 | 5 Hz - 50 Hz | 带宽最低，确保系统稳定 |

## 位置环PID控制器代码实现 (C语言)

以下是一个包含抗饱和和前馈的增量式位置PID控制器实现：

```c
/**
 * 位置环PID控制器结构体
 */
typedef struct {
    float Kp;           // 比例系数
    float Ki;           // 积分系数
    float Kd;           // 微分系数
    float Kff_v;        // 速度前馈系数
    float Kff_a;        // 加速度前馈系数 (可选)
    
    float target;       // 目标位置 (单位: 弧度或编码器计数)
    float feedback;     // 实际位置反馈
    float error;        // 当前误差
    float error_prev;   // 上一次误差
    float error_sum;    // 误差积分项
    float error_sum_max;// 积分限幅
    
    float output;       // 控制器输出 (作为速度环的输入)
    float output_max;   // 输出限幅 (最大速度指令)
    float output_min;   // 输出限幅 (最小速度指令)
    
    float dt;           // 控制周期 (秒)
} PositionPID_t;

/**
 * 位置环PID计算函数 (增量式，带抗饱和)
 * @param pid PID控制器实例指针
 * @param target 目标位置
 * @param feedback 位置反馈
 * @return 速度指令输出
 */
float PositionPID_Calculate(PositionPID_t *pid, float target, float feedback) {
    // 更新目标值和反馈值
    pid->target = target;
    pid->feedback = feedback;
    
    // 计算误差
    pid->error_prev = pid->error;
    pid->error = pid->target - pid->feedback;
    
    // 比例项
    float P_out = pid->Kp * pid->error;
    
    // 积分项 (带抗饱和)
    pid->error_sum += pid->error * pid->dt;
    // 积分限幅
    if (pid->error_sum > pid->error_sum_max) {
        pid->error_sum = pid->error_sum_max;
    } else if (pid->error_sum < -pid->error_sum_max) {
        pid->error_sum = -pid->error_sum_max;
    }
    float I_out = pid->Ki * pid->error_sum;
    
    // 微分项 (对误差微分，或对反馈微分以抑制设定值突变)
    float derivative = (pid->error - pid->error_prev) / pid->dt;
    float D_out = pid->Kd * derivative;
    
    // 前馈项 (速度前馈，假设目标速度已知或通过微分得到)
    static float prev_target = 0;
    float target_velocity = (target - prev_target) / pid->dt;
    prev_target = target;
    float FF_out = pid->Kff_v * target_velocity;
    
    // PID + 前馈 输出
    pid->output = P_out + I_out + D_out + FF_out;
    
    // 输出限幅 (作为速度环的指令)
    if (pid->output > pid->output_max) {
        pid->output = pid->output_max;
        // 抗饱和：若输出饱和，则停止积分 (Clamping)
        if (pid->error * pid->output > 0) {
            pid->error_sum -= pid->error * pid->dt; // 回退积分
        }
    } else if (pid->output < pid->output_min) {
        pid->output = pid->output_min;
        if (pid->error * pid->output > 0) {
            pid->error_sum -= pid->error * pid->dt;
        }
    }
    
    return pid->output;
}

/**
 * 位置环初始化函数
 */
void PositionPID_Init(PositionPID_t *pid, float kp, float ki, float kd, float dt) {
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->Kff_v = 0.0f; // 默认无前馈
    pid->Kff_a = 0.0f;
    
    pid->target = 0;
    pid->feedback = 0;
    pid->error = 0;
    pid->error_prev = 0;
    pid->error_sum = 0;
    pid->error_sum_max = 1000.0f; // 根据实际设置
    
    pid->output = 0;
    pid->output_max = 100.0f;     // 最大速度指令 (rad/s)
    pid->output_min = -100.0f;
    
    pid->dt = dt;
}
```

## 三环伺服系统实现步骤

### 步骤一：硬件与传感器准备

确保具备高分辨率位置传感器（如光电编码器、磁编码器、旋转变压器）。计算编码器线数，确定位置测量精度（例如：17位编码器，每圈131072个脉冲）。

### 步骤二：电流环与速度环调试完成

确保内环（电流环）和中环（速度环）已稳定工作，带宽满足要求。这是位置环稳定的基础。

### 步骤三：位置测量与校准

实现编码器读数、多圈计数、零位校准。处理编码器溢出。提供绝对位置或相对位置信息。

```c
// 示例：获取电机电角度 (结合编码器与极对数)
float Get_Electrical_Angle(void) {
    int32_t raw_count = Read_Encoder(); // 读取编码器原始值
    static int32_t prev_count = 0;
    static int32_t total_count = 0;
    
    // 处理溢出与多圈计数 (假设是增量式编码器)
    int32_t delta = raw_count - prev_count;
    if(delta > ENCODER_MAX/2) delta -= ENCODER_MAX; // 负向溢出
    else if(delta < -ENCODER_MAX/2) delta += ENCODER_MAX; // 正向溢出
    total_count += delta;
    prev_count = raw_count;
    
    // 计算机械角度 (弧度)
    float mechanical_angle = (float)total_count / ENCODER_RESOLUTION * 2.0f * PI;
    // 转换为电角度
    float electrical_angle = mechanical_angle * MOTOR_POLE_PAIRS;
    return electrical_angle;
}
```

### 步骤四：位置环PID参数初步整定

先将Ki和Kd设为0，逐渐增大Kp直到系统开始振荡，然后取其50%~70%作为初步P值。然后加入少量积分I消除静差。微分D用于抑制超调，需谨慎添加。

### 步骤五：加入前馈与高级功能

为提升动态性能，加入速度前馈（Kff_v）。对于轨迹跟踪，可加入加速度前馈（Kff_a）。实现S曲线规划、位置滤波、防抖动等算法。

### 步骤六：系统联调与优化

测试阶跃响应、正弦跟踪、定位精度、重复定位精度、刚度（抗扰动）。使用示波器或上位机观察位置误差曲线，微调参数。

### ⚠️ 重要注意事项

- 稳定性优先 ：三环系统必须从内到外依次整定，确保内环稳定后再调外环。
- 带宽递减原则 ：位置环带宽 << 速度环带宽 << 电流环带宽，通常相差5-10倍。
- 传感器噪声处理 ：位置微分得速度会放大噪声，需对位置信号滤波或使用观测器。
- 机械谐振规避 ：位置环带宽应避开机械系统的谐振频率。
- 实时性保证 ：位置环控制周期需满足奈奎斯特采样定理，通常1-5ms。

## 典型性能指标与测试方法

### 静态指标

- 定位精度 ：实际位置与目标位置的最大偏差。
- 重复定位精度 ：多次到达同一位置的分散程度。
- 分辨率 ：系统能分辨的最小位置变化。
- 静态刚度 ：单位负载扰动引起的位移量。

### 动态指标

- 阶跃响应 ：上升时间、超调量、调节时间。
- 跟踪误差 ：正弦轨迹跟踪时的最大相位/幅度误差。
- 带宽 ：-3dB频率点，反映系统快速性。
- 抗扰动恢复时间 ：突加负载后回到目标位置的时间。
