# PVD 与 BOR 欠压保护

[← 返回 Cortex-M4内核原理](./MOC.md) | [← 主页](../../index.md)

> 芯片供电电压下降时的两级硬件防线：先 PVD 软件抢救，再 BOR 硬件复位。

---

## 两级防线

```
VDD 下降
  │
  ├─ 低于 PVD 阈值（可编程）──▶ PVDO 置位 ──▶ PVD 中断 ──▶ 软件抢救（存 Flash）
  │
  └─ 低于 BOR 阈值（硬件）────▶ 硬件电路直接复位 ──▶ 防止程序跑飞/误写
```

| 阶段 | 阈值 | 谁介入 | 能做什么 |
| --- | --- | --- | --- |
| PVD | 可编程（软件设） | 软件（中断） | 进中断写代码：把重要数据存 Flash |
| BOR | 硬件固定（选项字节配） | 硬件电路 | 直接复位，不经过软件 |

---

## PVD（可编程电压检测）

PVD（Programmable Voltage Detector）属于 STM32 的 **PWR 电源控制模块**，实时监控 VDD：

| 项 | 说明 |
| --- | --- |
| 阈值 | 可编程，PWR->CR 的 PLS[2:0] 位选档（约 2.2V~2.9V，具体看型号） |
| 标志 | VDD < 阈值时 PWR->CSR 的 PVDO 置 1 |
| 中断 | 挂到 EXTI 线 16，配置后下降沿触发中断 |
| 用途 | 电压还没低到崩溃，留出时间窗口给软件「抢救数据」 |

---

## BOR（欠压复位）

BOR（Brown-Out Reset）是**纯硬件**防线，阈值比 PVD 更低：

| 项 | 说明 |
| --- | --- |
| 阈值 | 硬件固定，选项字节 BOR_LEV 配档 |
| 动作 | VDD < 阈值 → 硬件直接触发复位（电压恢复前保持复位） |
| 作用 | 电压低到 CPU 可能跑飞、Flash 误写时，强制复位，防止程序乱跑 |

> 一句话分工：**PVD 给软件「临死前存数据」的机会，BOR 兜底保证不会带病运行。**

---

## HAL 使用（PVD 保存数据）

```c
// 配置 PVD 阈值 + 使能中断
PWR_PVDTypeDef pvd;
pvd.PVDLevel = PWR_PVDLEVEL_5;              // 选阈值档位（如 2.5V）
pvd.Mode     = PWR_PVD_MODE_IT_FALLING;     // 电压跌破阈值时中断
HAL_PWR_ConfigPVD(&pvd);
HAL_PWR_EnablePVD();

HAL_NVIC_SetPriority(PVD_IRQn, 0, 0);
HAL_NVIC_EnableIRQ(PVD_IRQn);

// PVD 中断：抓紧存重要数据到 Flash
void PVD_IRQHandler(void)
{
    HAL_PWR_PVD_IRQHandler();
}

void HAL_PWR_PVDCallback(void)
{
    Save_Important_Data_To_Flash();   // 把关键变量写进 Flash 备用区
}
```

> Flash 写需要时间且耗电，PVD 阈值比 BOR 高，就是为了给这一步留出电压下降的时间窗。
