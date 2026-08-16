# MPU 内存保护单元

[← 返回 Cortex-M4内核原理](./MOC.md) | [← 主页](../../index.md)

> Cortex-M 内核外设，只管「谁能不能访问哪块内存」，**不做地址翻译**。

---

## 作用

| 需求                                   | MPU 怎么解决                                                                 |
| -------------------------------------- | ---------------------------------------------------------------------------- |
| 任务 A 破坏 OS 内核 / 其他任务内存     | 给每个任务划分独立 region，越界访问触发**MemManage 异常**（HardFault） |
| 防止误写只读配置（向量表、外设寄存器） | 给该区域设只读/禁写权限                                                      |
| 防代码注入攻击                         | XN（Execute Never）禁止某块内存执行                                          |
| 栈溢出保护                             | 在栈底下方留一块不可访问的 guard region                                      |

**本质**：在 CPU 与总线之间插一道「门禁」，每次访存拿地址比对 region 表——命中则按该 region 权限放行/拦截，未命中按默认背景属性处理。

---

## 硬件特性（Cortex-M4）

| 项          | 规则                                                                       |
| ----------- | -------------------------------------------------------------------------- |
| region 数量 | 最多 8 个（0~7）                                                           |
| region 大小 | 2 的幂（32B ~ 4GB）                                                        |
| 起始地址    | 必须对齐到自身大小（如 32KB 区域必须 32KB 对齐）                           |
| 重叠优先级  | **编号越大优先级越高**（7 最高），重叠处高编号属性生效               |
| 背景 region | 未被任何 region 覆盖的地址：使能 PRIVDEFENA 则按默认特权属性，否则禁止访问 |

---

## 使用流程

裸寄存器：先按表配 4 个寄存器，再走 6 步：

| 寄存器       | 作用                                                                                     |
| ------------ | ---------------------------------------------------------------------------------------- |
| `MPU_CTRL` | 总开关：ENABLE（使能）、HFNMIENA（HardFault/NMI 中仍生效）、PRIVDEFENA（特权级默认背景） |
| `MPU_RNR`  | 选择要操作的 region 编号（0~7）                                                          |
| `MPU_RBAR` | 该 region 基地址（[31:5]）+ VALID（bit4，写 1 生效）+ region 号（[3:0]）                 |
| `MPU_RASR` | 大小 + 属性：XN、AP、TEX/S/C/B、SRD 子区域禁用、SIZE、ENABLE                             |

`MPU_RASR` 的 AP 权限字段（[26:24]）：

| AP[2:0] | 特权级 | 非特权级 |
| ------- | ------ | -------- |
| 0b000   | 禁止   | 禁止     |
| 0b001   | 读/写  | 禁止     |
| 0b010   | 读/写  | 只读     |
| 0b011   | 读/写  | 读/写    |
| 0b101   | 只读   | 禁止     |
| 0b110   | 只读   | 只读     |
| 0b111   | 只读   | 只读     |

1. `MPU->CTRL = 0` —— 配置前先关 MPU
2. `MPU->RNR = n` —— 选 region 号
3. `MPU->RBAR = (addr & 0xFFFFFFE0) | 0x10` —— 设基地址（末 5 位清 0，bit4=VALID）
4. `MPU->RASR = ...` —— 设大小 + AP 权限 + 使能该 region
5. `MPU->CTRL = 1` —— 使能 MPU
6. 补 `__DSB(); __ISB();` 保证设置立即生效（见 [内存屏障](内存屏障与指令重排序.md)）

ST HAL / CMSIS：同一套配置封装成结构体 + 两行 API：

```c
MPU_Region_InitTypeDef MPU_InitStruct;

MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
MPU_InitStruct.BaseAddress      = 0x20000000;            // 起始地址，需对齐到 Size
MPU_InitStruct.Size             = MPU_REGION_SIZE_32KB;
MPU_InitStruct.SubRegionDisable = 0;                     // 全部子区域启用
MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS; // 特权/非特权均可读写
MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;

HAL_MPU_ConfigRegion(&MPU_InitStruct);
HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);   // 内部已自动 | ENABLE 位并加 DSB/ISB
```

---

## MPU vs MMU

| 维度         | MPU                             | MMU                                |
| ------------ | ------------------------------- | ---------------------------------- |
| 全称         | Memory**Protection** Unit | Memory**Management** Unit    |
| 核心功能     | 访问**权限检查**          | 虚拟地址 → 物理地址**翻译** |
| 地址是否变化 | 不变（直接用物理地址）          | 变（逻辑→物理）                   |
| 页表 / TLB   | 无                              | 有                                 |
| 虚拟内存     | 不支持                          | 支持                               |
| 典型场景     | MCU（Cortex-M）+ RTOS/裸机      | 应用处理器（Cortex-A）+ Linux      |
| 复杂度       | 简单（≤8 个 region）           | 复杂（多级页表）                   |
