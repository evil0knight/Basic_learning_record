# Park变换

[← 返回 FOC MOC](./MOC.md) | [← 返回主页](../../index.md)

---

## 一、Park变换的核心思想

两相静止坐标系（α-β）通过一个电角度$\theta_e$转换成两相旋转坐标系（d-q坐标系） 。

* **输入：** 静止坐标系电流 **$I_\alpha, I_\beta$**、电角度 **$\theta_e$**
* **处理：** 随转子旋转的坐标系投影变换
* **输出：** 直流旋转电流反馈值 **$I_d$**（直轴/励磁电流）、**$I_q$**（交轴/转矩电流）

## 二、数学推导

### 2.1 几何关系推导

设静止坐标系（α-β）中有一个空间矢量 $I_s = [I_\alpha, I_\beta]^T$。旋转坐标系（d-q）以电角速度 $\omega_e$ 逆时针旋转，其d轴与α轴的瞬时夹角为电角度 $\theta_e = \omega_e t$。

将矢量 $I_s$ 分别向d轴和q轴投影，利用三角函数关系，可以得到其在旋转坐标系下的分量：

$$
\begin{aligned}
I_d &= I_\alpha \cos\theta_e + I_\beta \sin\theta_e \\
I_q &= -I_\alpha \sin\theta_e + I_\beta \cos\theta_e
\end{aligned}
$$

- d轴（直轴） ：与转子永磁体磁场方向对齐，控制d轴电流（$I_d$）可以增强或削弱气隙磁场（用于弱磁控制）。
- q轴（交轴） ：超前d轴90度电角度，控制q轴电流（$I_q$）直接产生电磁转矩。在永磁同步电机（PMSM）中，转矩与 $I_q$ 成正比。
- 变换的核心 ：变换矩阵 $P(\theta)$ 中的角度 $\theta_e$ 必须是转子的实时电角度，通常由位置传感器（如编码器）或观测器（如滑模观测器）提供。

## 三、软件实现（C语言）

与 FOC 其他模块(Clarke/逆Park/SVPWM/电流环)风格统一，这里给出一份面向带FPU的 MCU 的浮点实现，MISRA 风格、带空指针校验。完整文件见 [相关代码/Park.c](../../库中车马多如簇/FOC/Park.c)

**优化要点：**

1. **一次性求 sin/cos**：Park 公式中 sin(θe) 和 cos(θe) 各出现两次，先算一遍存成局部变量再复用，避免调两次 `sinf`/`cosf`
2. **参数校验前置**：空指针直接返回错误码，避免访存异常。
3. **两种接口**：
   - `park_transform()`：内部直接 `sinf`/`cosf`，简单但较慢（约 60~200 个时钟周期，取决于平台）。
   - `park_transform_sc()`：由调用方传入预算好的 sin/cos。FOC 主循环里 Park 与反 Park 共用同一个 θe，查一次表拿到 sin/cos 分别喂给两个函数，省一次查表。
4. **三角函数查表优化**：Park 与反 Park 共用同一张 1024 点 sin LUT（位掩码取模自动处理负角度和多圈环绕），已独立成模块。LUT 以 `const` 常量形式**直接预算嵌入 Flash**，开机即可用，不需要运行时 init，也不占 RAM。控制中断里 `sin_lut_sincos(θe, &sin_t, &cos_t)` 一次拿到 sin/cos，分别喂给 `park_transform_sc()` 和 `inverse_park_sc()`，整个 FOC 循环只查一次表。

---

## 相关代码

- [Park.c](../../库中车马多如簇/FOC/Park.c)
- [sin_lut.c](../../库中车马多如簇/FOC/sin_lut.c) / [sin_lut.h](../../库中车马多如簇/FOC/sin_lut.h) — 共享 sin/cos 查表模块（与反 Park 复用）
