# PID

[←MOC](../MOC.md) | [←主页](../../index.md)
[← 返回电流环](../../术中自有万钟粟/FOC/电流环.md)

| 文件                    | 用途                              |
| ----------------------- | --------------------------------- |
| [pid知识.md](pid知识.md) | 位置式 vs 增量式对比与适用场景    |
| [PID.h](PID.h)           | 类型、配置、运行时与 4 个接口声明 |
| [PID.c](PID.c)           | 位置式/增量式实现与静态辅助       |

## 使用示例

### 单环示例:转速闭环驱动 PWM

目标:转一个电机到 `target` 转速,实际转速由编码器测得。误差 = target - 实际,PID 把误差转成 PWM 占空比。

```c
#include "PID.h"

/* ==================== 1. 文件顶部:全局静态定义 ==================== */

/* PID 运行时实例 —— 保存积分、历史误差等。
 * 一个控制环对应一个实例,不能两个电机共用一个。 */
static pid_t g_s_speed_pid;

/* PID 配置 —— 填完就不动了,所以加 const。
 * 所有带 . 的赋值用的是 C99 指定初始化,没写到的字段
 * 自动为 0 / false,即对应功能关闭。 */
static const pid_cfg_t g_s_speed_cfg = {
    .e_type      = PID_INCREMENT,  /* 增量式:电机速度环常用,
                                    * 切换目标时冲击小,无需单独抗饱和 */
    .f_kp        = 0.5f,           /* 比例增益,先定一个大致值,
                                    * 调试时从小往大加 */
    .f_ki        = 2.0f,           /* 积分增益,消除稳态误差 */
    .f_dt        = 0.001f,         /* 控制周期 1 ms = 1 kHz。
                                    * 必须和下面定时器的频率一致!
                                    * dt 填错 Ki/Kd 的行为就全错 */
    .f_out_max   = 1000.0f,        /* PWM 占空上限,与 motor_set_pwm
                                    * 可接受的范围对齐。
                                    * 填了这个就会自动限幅输出 */
    .f_dead_band = 1.0f            /* 误差小于 ±1 rpm 时输出 0,
                                    * 避免抖动。不要可以不填 */
};

/* 期望转速,外部任务(例如上位机通信或按键)修改。
 * volatile:中断和任务都会访问,防止编译器优化掉。
 * 单位 rpm,和编码器读出的单位必须一致。 */
static volatile float g_f_speed_target = 0.0f;

/* ==================== 2. 系统初始化(main 里调一次) ==================== */

void motor_init(void)
{
    /* 把配置拷进实例,清零所有历史。失败只会是参数非法,
     * 这里配置是写死的常量所以忽略返回值 */
    (void)pid_init(&g_s_speed_pid, &g_s_speed_cfg);

    /* 之后再启动定时器,保证中断进来时 PID 已就绪 */
}

/* ==================== 3. 定时中断里跑控制 ==================== */

/* 这个中断必须严格 1 ms 触发一次 —— 对应上面 f_dt = 0.001f。
 * 周期不准会直接影响微分和积分的效果 */
void TIM_IRQHandler(void)
{
    /* 第一步:读测量(反馈) */
    float f_speed_meas = encoder_read_rpm();

    /* 第二步:算控制量
     * 四个参数依次为:PID 实例、目标值、测量值、输出地址
     * 内部完成 e = target - measure,再按PID公式算 */
    float f_pwm = 0.0f;
    (void)pid_calc(&g_s_speed_pid,
                   g_f_speed_target, f_speed_meas, &f_pwm);

    /* 第三步:写执行器
     * f_pwm 已经被 PID 限在 [-1000, +1000] 内,
     * 直接传给驱动即可。符号表示方向。 */
    motor_set_pwm((int16_t)f_pwm);
}
```

### 双环级联示例:速度外环 + 电流内环

单环直接把速度映射到 PWM 太粗糙 —— 负载突变时电流瞬间失控。工业做法是**串两个 PID**:

- **外环(速度环)**:输入速度误差,输出"**我希望流多少电流**",电流作为内环的目标
- **内环(电流环)**:输入电流误差,输出 PWM 占空比,直接驱动

内环比外环快一个数量级:外环 1 ms 跑一次,内环 100 μs 跑一次。这样电流能快速跟上,外环只管慢慢调速度。

```c
/* ==================== 文件顶部 ==================== */

/* 两个独立的 PID 实例 —— 一环一个,不能共用 */
static pid_t g_s_speed_pid;      /* 外环:速度误差 -> 期望电流 */
static pid_t g_s_current_pid;    /* 内环:电流误差 -> PWM     */

/* 外环配置:慢环,控制速度 */
static const pid_cfg_t g_s_speed_cfg = {
    .e_type    = PID_POSITION,   /* 位置式:对输出饱和要有抗饱和处理,
                                  * 这里靠 f_int_max + 条件积分 */
    .f_kp      = 0.8f,
    .f_ki      = 3.0f,
    .f_dt      = 0.001f,         /* 1 ms 跑一次 */
    .f_out_max = 20.0f,          /* ★关键:外环输出就是"期望电流",
                                  * 必须限制在电机能承受的范围 ±20 A */
    .f_int_max = 20.0f           /* 积分值也限在 ±20 A,防止积分发散 */
};

/* 内环配置:快环,控制电流 */
static const pid_cfg_t g_s_current_cfg = {
    .e_type    = PID_INCREMENT,  /* 增量式:快环切换目标频繁,平滑 */
    .f_kp      = 1.5f,
    .f_ki      = 500.0f,         /* 内环 Ki 通常比外环大 1~2 个数量级 */
    .f_dt      = 0.0001f,        /* 100 us 跑一次,10 kHz */
    .f_out_max = 1000.0f         /* PWM 占空范围 */
};

/* 环间桥梁 —— 外环把算出来的"期望电流"写到这里,
 * 内环下一次运行时读走当作自己的 target。
 * volatile:外环和内环在不同中断/任务里访问,防止优化 */
static volatile float g_f_speed_target  = 0.0f;  /* 用户设定的转速目标 */
static volatile float g_f_cur_target    = 0.0f;  /* 外环写、内环读 */

/* ==================== 初始化 ==================== */

void motor_init(void)
{
    /* 两个实例分别初始化,顺序无所谓 */
    (void)pid_init(&g_s_speed_pid,   &g_s_speed_cfg);
    (void)pid_init(&g_s_current_pid, &g_s_current_cfg);
}

/* ==================== 外环:1 ms 周期任务(或低优先中断) ==================== */

void outer_loop_1ms(void)
{
    /* 第一步:读速度测量值 */
    float f_speed = encoder_read_rpm();

    /* 第二步:算出"我希望流多少电流"
     * 注意输出变量 f_new_cur_target 的物理含义是电流(安培),
     * 不是 PWM!因为外环的 f_out_max 填的是电流限幅。 */
    float f_new_cur_target = 0.0f;
    (void)pid_calc(&g_s_speed_pid,
                   g_f_speed_target, f_speed, &f_new_cur_target);

    /* 第三步:把结果交给内环
     * 外环没有"写执行器"的物理动作,这一步就是外环的"写". */
    g_f_cur_target = f_new_cur_target;
}

/* ==================== 内环:100 us 定时中断(高优先) ==================== */

void inner_loop_100us(void)
{
    /* 第一步:读电流测量值(ADC 采样) */
    float f_cur = adc_read_current();

    /* 第二步:以外环刚才给的 g_f_cur_target 为目标,算 PWM
     * 内环完全不知道速度环的存在,它只做电流闭环 */
    float f_pwm = 0.0f;
    (void)pid_calc(&g_s_current_pid,
                   g_f_cur_target, f_cur, &f_pwm);

    /* 第三步:写执行器 */
    motor_set_pwm((int16_t)f_pwm);
}
```

### 进阶示例:启用全部改进功能

调出能跑的基础参数后,把需要的改进字段逐条加上去。下面演示一个把常见改进都打开的位置式配置,**实际使用时按需裁剪**,不要无脑全开。

```c
/* ==================== 完整功能配置 ==================== */

static pid_t g_s_pid;

static const pid_cfg_t g_s_cfg = {
    /* ---------- 必填基础 ---------- */
    .e_type    = PID_POSITION,
    .f_kp      = 1.2f,
    .f_ki      = 2.0f,
    .f_kd      = 0.05f,
    .f_dt      = 0.001f,

    /* ---------- 输出相关 ---------- */
    .f_out_max   = 1000.0f,     /* 输出限幅:填了就会启用"条件积分"抗饱和   */
    .f_out_rc    = 0.005f,      /* 输出滤波:控制量毛刺大、执行器怕抖才加   */

    /* ---------- 积分相关(抗积分发散/超调) ---------- */
    .f_int_max   = 500.0f,      /* 积分限幅:积分收不住、系统明显超调时加   */
    .f_i_sep_err = 100.0f,      /* 积分分离:误差 > 100 时暂停积分,减大超调 */
    .f_vsi_a     = 50.0f,       /* 变速积分过渡宽度                        */
    .f_vsi_b     = 30.0f,       /* 变速积分全积分阈值                      */
                                /*   误差 ≤ 30: 全积分                     */
                                /*   30 < 误差 < 80: 线性衰减到 0          */
                                /*   误差 ≥ 80: 本次不积分                 */
    .b_trapezoid_i = true,      /* 梯形积分:高精度场合启用,否则不必加     */

    /* ---------- 微分相关(抗目标冲击/噪声) ---------- */
    .b_d_on_measure = true,     /* 微分先行:目标会阶跃时必加,几乎总是加   */
    .f_d_rc         = 0.01f,    /* 微分滤波:测量有噪声、Kd > 0 时建议加   */

    /* ---------- 比例相关 ---------- */
    .b_p_on_measure = false,    /* 比例先行:目标阶跃冲击太大时才加         */
                                /* 启用后相当于纯积分响应目标,慎用         */

    /* ---------- 死区 ---------- */
    .f_dead_band = 2.0f,        /* 稳态抖动大的系统填一个小值即可           */

    /* ---------- 堵转检测 ---------- */
    .u32_stall_count     = 500, /* 连续 500 个周期(本例 0.5 s)满足条件判堵 */
    .f_stall_out_ratio   = 0.9f,/* 输出 ≥ 90% 上限才算 "在使劲"              */
    .f_stall_target_min  = 10.0f,/* 目标 < 10 不检测,避免目标为零时误判     */
    .f_stall_err_ratio   = 0.5f,/* 相对误差超 50% 开始累计                   */
};

/* ==================== 中断里的完整用法 ==================== */

void TIM_IRQHandler(void)
{
    float f_target = get_target();
    float f_meas   = read_sensor();
    float f_u_req  = 0.0f;

    /* 1. 算控制量(已包含上面配置的全部改进) */
    (void)pid_calc(&g_s_pid, f_target, f_meas, &f_u_req);

    /* 2. 堵转保护:读 b_stalled 字段,库只告诉你,停不停自己决定 */
    if (g_s_pid.b_stalled) {
        motor_set_pwm(0);
        return;        /* 跳过写执行器,也不做饱和反馈 */
    }

    /* 3. 写执行器,假设底层驱动可能限幅,返回实际写入值 */
    float f_u_actual = motor_set_pwm_clamped(f_u_req);

    /* 4. 饱和反馈:当实际写入 < 请求时告知 PID,位置式会回退积分
     *    增量式会直接返回 OK 不处理,所以无脑调也安全 */
    if (fabsf(f_u_req) > 1e-6f) {
        float f_scale = fabsf(f_u_actual) / fabsf(f_u_req);
        (void)pid_sat_feedback(&g_s_pid, f_scale);
    }
}
```

要点:

- **全部功能开着不会出错,但也不见得更好** —— 变速积分、比例先行等用错了反而拖慢响应
- `b_d_on_measure` 几乎总建议开(目标阶跃时微分不会尖峰),这是免费午餐
- `pid_sat_feedback` 对增量式是 no-op,对位置式才有效 —— 两种类型都这么写也安全
- 堵转标志触发后不会自动清除,等条件消失(误差回落或输出降低)下一周期自动清零
