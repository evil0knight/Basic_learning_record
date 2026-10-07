# SVPWM代码实现

[← 返回本模块 MOC](./MOC.md) | [← 返回 FOC MOC](../MOC.md) | [← 返回主页](../../../index.md)

---

## 一、SVPWM算法核心思想回顾

空间矢量脉宽调制（SVPWM）是一种通过逆变器六个开关管的特定开关组合，在电机定子绕组中产生 逼近圆形旋转磁场 的PWM调制技术。其核心是将三相电压矢量投影到α-β坐标系，并利用六个非零基本矢量和两个零矢量进行合成。

图1：SVPWM基本矢量与扇区划分

## 二、SVPWM算法实现步骤

1. 坐标变换 ：将三相电压Ua, Ub, Uc通过Clarke变换得到α-β坐标系下的Uα, Uβ。
1. 扇区判断 ：根据Uα, Uβ计算三个参考量A, B, C，并通过符号判断确定当前矢量所在扇区（I~VI）。
1. 计算相邻矢量作用时间 ：根据扇区，利用Uα, Uβ和直流母线电压Vdc，计算两个相邻基本矢量的作用时间T1, T2。
1. 时间饱和处理 ：检查T1+T2是否超过PWM周期T，若超过则按比例缩小。
1. 计算PWM占空比 ：根据扇区、T1, T2和零矢量分配时间，计算三相占空比Ta, Tb, Tc。
1. 写入PWM比较寄存器 ：将占空比转换为定时器比较值，更新硬件寄存器，生成实际PWM波形。
**关键点：**扇区判断和时间计算是SVPWM算法的核心，直接影响波形质量和电机运行性能。
## 三、SVPWM扇区与时间对应表

| 扇区 | 判断条件 | 相邻矢量 | 作用时间公式 | 占空比计算 |
| --- | --- | --- | --- | --- |
| I | Uβ>0, Uα>0, |Uβ|<√3Uα | V₁, V₂ | T₁=K(√3Uα-Uβ), T₂=K·2Uβ | Ta=(T-T₁-T₂)/4, Tb=Ta+T₁, Tc=Tb+T₂ |
| II | Uβ>0, |Uβ|>√3|Uα| | V₂, V₃ | T₁=K(√3Uα+Uβ), T₂=K(-√3Uα+Uβ) | Ta=(T-T₁-T₂)/4, Tb=Ta+T₁, Tc=Tb+T₂ |
| III | Uα<0, Uβ>0, |Uβ|<-√3Uα | V₃, V₄ | T₁=K(-√3Uα-Uβ), T₂=K·2Uβ | Ta=(T-T₁-T₂)/4, Tb=Ta+T₁, Tc=Tb+T₂ |
| IV | Uβ<0, Uα<0, |Uβ|<√3Uα | V₄, V₅ | T₁=K(-√3Uα+Uβ), T₂=K·(-2Uβ) | Ta=(T-T₁-T₂)/4, Tb=Ta+T₁, Tc=Tb+T₂ |
| V | Uβ<0, |Uβ|>-√3Uα | V₅, V₆ | T₁=K(√3Uα-Uβ), T₂=K(-√3Uα-Uβ) | Ta=(T-T₁-T₂)/4, Tb=Ta+T₁, Tc=Tb+T₂ |
| VI | Uα>0, Uβ<0, |Uβ|>√3Uα | V₆, V₁ | T₁=K(√3Uα+Uβ), T₂=K·(-2Uβ) | Ta=(T-T₁-T₂)/4, Tb=Ta+T₁, Tc=Tb+T₂ |

注：$K = T / V_{dc}$，$T$ 为PWM周期，$V_{dc}$ 为直流母线电压。

## 四、SVPWM软件架构图

## 五、C语言代码实现（适用于STM32等MCU）

### svpwm.h - 头文件

```c
#ifndef __SVPWM_H
#define __SVPWM_H

#include "stdint.h"

// SVPWM配置结构体
typedef struct {
    float Ualpha;       // α轴电压
    float Ubeta;        // β轴电压
    float Vdc;          // 直流母线电压
    float Tpwm;         // PWM周期时间（秒）
    uint16_t period;    // PWM定时器周期值
    uint16_t CCRa;      // 相位A比较值
    uint16_t CCRb;      // 相位B比较值
    uint16_t CCRc;      // 相位C比较值
    uint8_t sector;     // 当前扇区（1~6）
} SVPWM_HandleTypeDef;

// 函数声明
void SVPWM_Init(SVPWM_HandleTypeDef *hsvpwm, float Vdc, float Tpwm, uint16_t period);
void SVPWM_Calc(SVPWM_HandleTypeDef *hsvpwm);
uint8_t SVPWM_GetSector(float Ualpha, float Ubeta);

#endif
```

### svpwm.c - 源文件

```c
#include "svpwm.h"
#include "math.h"

#define SQRT3 1.73205080757f
#define ONE_BY_SQRT3 0.57735026919f
#define TWO_THIRD 0.66666666667f

/**
  * @brief  初始化SVPWM模块
  * @param  hsvpwm: SVPWM句柄指针
  * @param  Vdc: 直流母线电压
  * @param  Tpwm: PWM周期时间（秒）
  * @param  period: PWM定时器周期值
  * @retval 无
  */
void SVPWM_Init(SVPWM_HandleTypeDef *hsvpwm, float Vdc, float Tpwm, uint16_t period)
{
    hsvpwm->Vdc = Vdc;
    hsvpwm->Tpwm = Tpwm;
    hsvpwm->period = period;
    hsvpwm->Ualpha = 0.0f;
    hsvpwm->Ubeta = 0.0f;
    hsvpwm->sector = 0;
    hsvpwm->CCRa = period / 2;
    hsvpwm->CCRb = period / 2;
    hsvpwm->CCRc = period / 2;
}

/**
  * @brief  判断扇区
  * @param  Ualpha: α轴电压
  * @param  Ubeta: β轴电压
  * @retval 扇区编号（1~6）
  */
uint8_t SVPWM_GetSector(float Ualpha, float Ubeta)
{
    uint8_t sector = 0;
    
    // 计算参考值
    float A = Ubeta;
    float B = -Ubeta + SQRT3 * Ualpha;
    float C = -Ubeta - SQRT3 * Ualpha;
    
    // 判断扇区
    if(A > 0.0f) sector |= 0x01;
    if(B > 0.0f) sector |= 0x02;
    if(C > 0.0f) sector |= 0x04;
    
    // 映射到1~6扇区
    switch(sector)
    {
        case 1: return 2;  // 010 -> 扇区II
        case 2: return 6;  // 100 -> 扇区VI
        case 3: return 1;  // 110 -> 扇区I
        case 4: return 4;  // 001 -> 扇区IV
        case 5: return 3;  // 011 -> 扇区III
        case 6: return 5;  // 101 -> 扇区V
        default: return 0; // 错误
    }
}

/**
  * @brief  计算SVPWM占空比
  * @param  hsvpwm: SVPWM句柄指针
  * @retval 无
  */
void SVPWM_Calc(SVPWM_HandleTypeDef *hsvpwm)
{
    float Ualpha = hsvpwm->Ualpha;
    float Ubeta = hsvpwm->Ubeta;
    float Vdc = hsvpwm->Vdc;
    float T = hsvpwm->Tpwm;
    uint16_t period = hsvpwm->period;
    
    float T1, T2, T0;
    float Ta, Tb, Tc;
    float X, Y, Z;
    
    // 1. 扇区判断
    hsvpwm->sector = SVPWM_GetSector(Ualpha, Ubeta);
    
    // 2. 计算中间变量
    float K = T / Vdc;
    X = SQRT3 * Ubeta * K;
    Y = (1.5f * Ualpha + 0.5f * SQRT3 * Ubeta) * K;
    Z = (-1.5f * Ualpha + 0.5f * SQRT3 * Ubeta) * K;
    
    // 3. 根据扇区计算T1, T2
    switch(hsvpwm->sector)
    {
        case 1: // 扇区I
            T1 = Z;
            T2 = X;
            break;
        case 2: // 扇区II
            T1 = Y;
            T2 = -Z;
            break;
        case 3: // 扇区III
            T1 = -X;
            T2 = Y;
            break;
        case 4: // 扇区IV
            T1 = -Z;
            T2 = -X;
            break;
        case 5: // 扇区V
            T1 = -Y;
            T2 = Z;
            break;
        case 6: // 扇区VI
            T1 = X;
            T2 = -Y;
            break;
        default:
            T1 = 0;
            T2 = 0;
            break;
    }
    
    // 4. 时间饱和处理
    if((T1 + T2) > T)
    {
        T1 = T1 * T / (T1 + T2);
        T2 = T2 * T / (T1 + T2);
    }
    T0 = T - T1 - T2;  // 零矢量时间
    
    // 5. 计算占空比
    float Tcmax = period;
    switch(hsvpwm->sector)
    {
        case 1:
            Ta = (T - T1 - T2) / 4.0f;
            Tb = Ta + T1;
            Tc = Tb + T2;
            break;
        case 2:
            Ta = (T - T1 - T2) / 4.0f + T1;
            Tb = Ta - T1;
            Tc = Tb + T2;
            break;
        case 3:
            Ta = (T - T1 - T2) / 4.0f + T1 + T2;
            Tb = Ta - T1 - T2;
            Tc = Tb + T2;
            break;
        case 4:
            Ta = (T - T1 - T2) / 4.0f + T1 + T2;
            Tb = Ta - T2;
            Tc = Tb - T1;
            break;
        case 5:
            Ta = (T - T1 - T2) / 4.0f + T2;
            Tb = Ta - T2;
            Tc = Tb - T1;
            break;
        case 6:
            Ta = (T - T1 - T2) / 4.0f;
            Tb = Ta + T2;
            Tc = Tb + T1;
            break;
        default:
            Ta = T / 2.0f;
            Tb = T / 2.0f;
            Tc = T / 2.0f;
            break;
    }
    
    // 6. 转换为PWM比较值（中心对齐模式）
    hsvpwm->CCRa = (uint16_t)((Ta / T) * period);
    hsvpwm->CCRb = (uint16_t)((Tb / T) * period);
    hsvpwm->CCRc = (uint16_t)((Tc / T) * period);
}
```

## 六、Python代码实现（用于仿真验证）

### svpwm_sim.py - Python仿真代码

```c
import numpy as np
import matplotlib.pyplot as plt

class SVPWM_Simulator:
    def __init__(self, Vdc=24.0, Tpwm=1e-3, fs=20000):
        """
        初始化SVPWM仿真器
        :param Vdc: 直流母线电压(V)
        :param Tpwm: PWM周期(s)
        :param fs: 采样频率(Hz)
        """
        self.Vdc = Vdc
        self.Tpwm = Tpwm
        self.fs = fs
        self.sector = 0
        self.sqrt3 = np.sqrt(3)
        
    def get_sector(self, Ualpha, Ubeta):
        """判断扇区"""
        A = Ubeta
        B = -Ubeta + self.sqrt3 * Ualpha
        C = -Ubeta - self.sqrt3 * Ualpha
        
        sector_map = {1:2, 2:6, 3:1, 4:4, 5:3, 6:5}
        sector_code = 0
        if A > 0: sector_code |= 0x01
        if B > 0: sector_code |= 0x02
        if C > 0: sector_code |= 0x04
        
        return sector_map.get(sector_code, 0)
    
    def calculate(self, Ualpha, Ubeta):
        """计算SVPWM占空比"""
        # 扇区判断
        self.sector = self.get_sector(Ualpha, Ubeta)
        
        # 计算中间变量
        K = self.Tpwm / self.Vdc
        X = self.sqrt3 * Ubeta * K
        Y = (1.5 * Ualpha + 0.5 * self.sqrt3 * Ubeta) * K
        Z = (-1.5 * Ualpha + 0.5 * self.sqrt3 * Ubeta) * K
        
        # 计算T1, T2
        T1, T2 = 0, 0
        if self.sector == 1:
            T1, T2 = Z, X
        elif self.sector == 2:
            T1, T2 = Y, -Z
        elif self.sector == 3:
            T1, T2 = -X, Y
        elif self.sector == 4:
            T1, T2 = -Z, -X
        elif self.sector == 5:
            T1, T2 = -Y, Z
        elif self.sector == 6:
            T1, T2 = X, -Y
        
        # 时间饱和处理
        if (T1 + T2) > self.Tpwm:
            scale = self.Tpwm / (T1 + T2)
            T1 *= scale
            T2 *= scale
        
        T0 = self.Tpwm - T1 - T2  # 零矢量时间
        
        # 计算占空比
        if self.sector == 1:
            Ta = (self.Tpwm - T1 - T2) / 4.0
            Tb = Ta + T1
            Tc = Tb + T2
        elif self.sector == 2:
            Ta = (self.Tpwm - T1 - T2) / 4.0 + T1
            Tb = Ta - T1
            Tc = Tb + T2
        elif self.sector == 3:
            Ta = (self.Tpwm - T1 - T2) / 4.0 + T1 + T2
            Tb = Ta - T1 - T2
```

