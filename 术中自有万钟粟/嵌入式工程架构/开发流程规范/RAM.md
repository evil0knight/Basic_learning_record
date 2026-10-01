# 需求分配矩阵（RAM）

[← 开发流程规范](./MOC.md) | [← 主页](../../../index.md)

> 来源：[Stage4 02 需求分配矩阵 Requirement Allocation Matrix（RAM）](https://twd6onxsxva.feishu.cn/docx/HaU7dQS2poyKYix6fKFcXlm1nMf)

---

| 字段 | 内容 |
| --- | --- |
| 文档编号 | EC-WATCH-RAM-001 |
| 阶段 | Stage 4 – System Architecture / Allocation |
| Owner（主责） | 系统架构师（System Architect）/ 系统工程师（System Engineer） |
| 参与者 | 产品经理、嵌入式负责人、BLE负责人、App负责人、硬件负责人、测试/产测负责人 |
| 输入文档 | Stage1 SND/UND；Stage2 KAD/QFD/BRD；Stage3 SyRS；Stage4 SAD/ADR/SID |
| 输出文档 | RAM（本文件） |
| 适用范围 | MVP~R2（以SyRS目标版本字段为准） |
| 关键范围约束 | NFC：Out of Scope；定位：手机端GPS获取后通过BLE同步；受限网络：离线自治 + 有网补传 |

## 0. 修订记录

| 版本 | 日期 | 变更说明 | 作者 | 审核 |
| --- | --- | --- | --- | --- |
| V1.0 | YYYY-MM-DD | 初版：基于SyRS全量SR建立分配矩阵 |  |  |

## 1. 文档目的（Purpose）

RAM 的目的不是“写需求”，而是把 SyRS 中每一条系统需求（SR）明确分配到：主落点模块（Primary Domain）、分配对象（To）、唯一责任方（Owner）、协同边界（Co-module & Interface）、验证方式（V&V）。从而避免职责不清、接口无人负责、验证无闭环等常见问题。

## 2. 分配范围与系统边界（Scope & Boundary）

### 2.1 本项目关键边界（写入RAM基线）

- NFC：不在范围内（Out of Scope）。
- 定位：不做手表端独立GPS；默认由手机端获取GPS，通过BLE下发/同步给手表；无手机时允许降级为“无定位/近场RSSI”。
- 受限网络：系统必须具备离线自治能力（采集/判断/本地告警/记录），有网则补传/上报。

### 2.2 RAM 与后续文档关系

SyRS（SR）描述系统做什么；RAM 解决谁做、做在哪、怎么协作、怎么验证。Stage5（SSRD/SRS）将进一步把分配细化为软件/硬件可实现条目；Stage6（SDD）描述设计；Stage7（IVV）完成验证闭环。

## 3. 术语与缩写（Glossary）

| 缩写 | 中文 | 说明 |
| --- | --- | --- |
| SR | System Requirement | 系统需求（SyRS条目） |
| RAM | Requirement Allocation Matrix | 需求分配矩阵（本文） |
| SAD | System Architecture Document | 系统架构文档 |
| SID | System Interface Definition | 系统接口定义 |
| ADR | Architecture Decision Record | 架构决策记录 |
| V&V | Verification & Validation | 验证与确认（Test/Analysis/Inspection/Demo） |
| Primary / Co-module | 主落点 / 协同模块 | 主责任域 vs 需要接口协作的域 |

## 4. 一级模块能力域（System Domains）与责任边界

为保证分配可执行，本项目采用“一级模块（系统能力域）→ 分配对象（工程落点）→ 组件/接口”的三层拆分。

| 一级模块 | 范围/能力域 | 主责Owner |
| --- | --- | --- |
| SYS-A 电源与低功耗 | 电池/充电/上电关机、低功耗策略、班次续航目标 | 硬件电源负责人 + 嵌入式低功耗负责人 |
| SYS-B 生命体征与运动 | PPG/体温/IMU采集、滤波/阈值判定、事件生成 | 嵌入式传感负责人 + 算法/数据负责人 |
| SYS-C 人机交互与显示 | LVGL界面、触控/按键、蜂鸣/震动、告警呈现 | 嵌入式UI负责人 + 产品交互 |
| SYS-D 数据存储与日志 | 外部Flash、事件/趋势数据、日志导出、掉电保护 | 嵌入式存储负责人 |
| SYS-E 无线通信与互联 | BLE连接/GATT、广播、STM32↔nRF串口协议 | BLE负责人 + 嵌入式通信负责人 |
| SYS-F 安全与OTA | 升级/回滚、完整性校验、鉴权、密钥策略 | 安全/Bootloader负责人 |
| SYS-G 手机与平台 | 手机App（配置/查看/定位提供/升级代理）、云平台（可选） | 移动端负责人（+后端负责人） |
| SYS-H 生产测试与运维 | 产测模式、校准流程、工装与脚本、现场诊断 | 测试/产测负责人 |
| SYS-I 可靠性与合规 | ESD/EMC/IP等级、寿命/可靠性、风险与认证 | QA/可靠性负责人 + 硬件 |
| SYS-Z 系统约束与范围 | 受限网络约束、范围边界、系统级非功能约束统筹 | 系统工程/产品 |

## 5. 分配原则（Allocation Principles）

### 5.1 分配原则（保留原稿并扩展为可执行版本）

1. 能纯软件实现的，不推给硬件。
2. 能通过硬件降低复杂度的，不硬顶软件。
3. 责任必须唯一明确，不能“大家一起”。
4. 接口必须有人负责：跨域接口必须在SID中有条目，并在RAM中标注接口Owner。
5. 可验证优先：每条SR必须给出V&V方式，否则不进入基线。
6. 风险前置：救命链路（告警、断网自治、OTA回滚、低功耗）优先分配与验证。
7. 版本化：RAM必须标注目标版本（MVP/R1/R2），防止范围膨胀。

## 6. 需求分配矩阵（RAM）

说明：下表按 SyRS 的 SR 结构整理；用于 Stage5（SSRD/SRS）继续细化。

| SR ID | 类型 | P | 主落点模块 | 协同模块 | 分配对象（To） | Owner（唯一） | V&V | 追溯（BR/UN） |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| SR-001 | 功能 | P0 | SYS-B | SYS-C | STM32固件 + 传感器硬件 | 传感/固件负责人 | Test | BR-001/UN-001 |
| SR-002 | 功能 | P0 | SYS-B | SYS-C | STM32固件 + 传感器硬件 | 传感/固件负责人 | Test | BR-001/UN-001 |
| SR-003 | 功能 | P0 | SYS-B |  | STM32固件 | 算法/固件负责人 | Test/Analysis | BR-001 |
| SR-004 | 功能 | P0 | SYS-B | SYS-G | STM32固件 + App配置 | 算法/固件负责人 | Test | BR-002/UN-002 |
| SR-005 | 功能 | P0 | SYS-C | SYS-B | STM32固件 + 外设硬件 | UI/固件负责人 | Test | BR-002/UN-002 |
| SR-006 | 性能 | P0 | SYS-B | SYS-C | STM32固件 | 系统工程 + 固件负责人 | Test | BR-002 |
| SR-007 | 功能 | P0 | SYS-C | SYS-E | STM32固件 + nRF固件 | UI/固件负责人 | Test | BR-003/UN-003 |
| SR-008 | 功能 | P0 | SYS-D | SYS-C | STM32固件 + 外部Flash | 存储/固件负责人 | Test | BR-003 |
| SR-009 | 功能 | P1 | SYS-E |  | nRF固件 | BLE负责人 | Test | BR-002/UN-003 |
| SR-010 | 可靠性 | P1 | SYS-E | SYS-D | nRF固件 + STM32日志 | BLE负责人 | Test | BR-002 |
| SR-011 | 功能 | P1 | SYS-G | SYS-E/SYS-C | App + BLE + STM32提示 | App负责人 | Test | BR-004/UN-004 |
| SR-012 | 功能 | P1 | SYS-C | SYS-D | STM32固件 | UI/固件负责人 | Test | BR-004 |
| SR-013 | 接口 | P1 | SYS-G | SYS-E | App + BLE | App负责人 | Test | BR-004 |
| SR-014 | 功能 | P1 | SYS-G | SYS-E | App配置 + BLE下发 | App负责人 | Test | BR-004 |
| SR-015 | 约束 | P1 | SYS-Z | SYS-B/C/D | 系统级（STM32固件） | 系统工程师 | Analysis/Test | BR-001 |
| SR-016 | 功能 | P1 | SYS-D | SYS-E/G | STM32固件 + App | 存储/固件负责人 | Test | BR-001 |
| SR-017 | 功能 | P1 | SYS-D | SYS-E/G | STM32固件 + App工具 | 存储/固件负责人 | Test | BR-005/UN-007 |
| SR-018 | 功能 | P1 | SYS-D |  | STM32固件 | 存储/固件负责人 | Test | BR-005 |
| SR-019 | 功能 | P1 | SYS-F | SYS-E/G | STM32 BL + nRF DFU + App代理 | 安全/BL负责人 | Test | BR-005/UN-008 |
| SR-020 | 安全 | P1 | SYS-F | SYS-D | Bootloader/OTA | 安全/BL负责人 | Test | BR-005 |
| SR-021 | 可靠性 | P1 | SYS-F | SYS-E/G | OTA链路 | 安全/BL负责人 | Test | BR-005 |
| SR-022 | 功能 | P2 | SYS-H | SYS-B/D/A | STM32固件 | 测试/固件负责人 | Test | BR-005 |
| SR-023 | 功能 | P2 | SYS-D | SYS-E/G | STM32 + App | 存储/固件负责人 | Demo/Test | BR-005 |
| SR-024 | 接口 | P0 | SYS-E |  | STM32固件 + nRF固件 | BLE负责人（接口Owner） | Inspection/Test | BR-002 |
| SR-025 | 接口 | P1 | SYS-E | SYS-G | App + nRF + STM32 | BLE负责人 | Test | BR-001/004 |
| SR-026 | 接口 | P1 | SYS-G | SYS-E/D | App + BLE | App负责人 | Test | BR-005 |
| SR-027 | 接口 | P1 | SYS-G | SYS-E | App + BLE | App负责人 | Test | BR-001 |
| SR-028 | 接口 | P2 | SYS-G | SYS-E | App/网关/可选平台 | 系统工程师 | Inspection | BR-006 |
| SR-029 | 性能 | P0 | SYS-C | SYS-B | STM32固件 | UI/固件负责人 | Test | BR-002 |
| SR-030 | 性能 | P1 | SYS-A | SYS-B | STM32固件 | 低功耗负责人 | Analysis | BR-001 |
| SR-031 | 性能 | P1 | SYS-E |  | nRF固件 | BLE负责人 | Test | BR-002 |
| SR-032 | 性能 | P2 | SYS-D | SYS-G | STM32 + App | 存储负责人 | Test | BR-005 |
| SR-033 | 可靠性 | P0 | SYS-I | SYS-A | STM32固件 | 固件负责人 | Test | BR-001 |
| SR-034 | 可靠性 | P1 | SYS-D | SYS-I | STM32固件 | 存储/固件负责人 | Test | BR-005 |
| SR-035 | 可靠性 | P1 | SYS-E | SYS-D | nRF/STM32 | BLE负责人 | Test | BR-002 |
| SR-036 | 可靠性 | P1 | SYS-D | SYS-C | STM32固件 | 存储负责人 | Test | BR-005 |
| SR-037 | 可靠性 | P2 | SYS-E |  | nRF固件 | BLE负责人 | Analysis/Test | BR-002 |
| SR-038 | 功能 | P0 | SYS-A | SYS-C | STM32固件 + 电源硬件 | 低功耗负责人 | Test | BR-001 |
| SR-039 | 功能 | P0 | SYS-A | SYS-B/C | STM32固件 | 低功耗负责人 | Test | BR-001 |
| SR-040 | 目标 | P1 | SYS-A | SYS-I | 系统级（HW+FW） | 系统工程+硬件负责人 | Analysis/Test | BR-001 |
| SR-041 | 功能 | P1 | SYS-A |  | STM32固件 | 低功耗负责人 | Test | BR-001 |
| SR-042 | 安全 | P1 | SYS-F |  | Bootloader/OTA | 安全/BL负责人 | Inspection/Test | BR-005 |
| SR-043 | 安全 | P1 | SYS-F | SYS-E/G | BL + BLE | 安全负责人 | Inspection/Test | BR-005 |
| SR-044 | 安全 | P2 | SYS-G | SYS-F | App + 可选平台 | App/后端负责人 | Inspection | BR-005 |
| SR-045 | 架构 | P1 | SYS-Z | SYS-C/D/E | STM32固件 | 嵌入式负责人 | Inspection | BR-006 |
| SR-046 | 架构 | P1 | SYS-Z |  | STM32固件 | 嵌入式负责人 | Inspection | BR-006 |
| SR-047 | 架构 | P2 | SYS-B | SYS-Z | STM32固件 | 嵌入式负责人 | Inspection | BR-006 |
| SR-048 | 架构 | P2 | SYS-E | SYS-G | BLE/串口协议 | BLE负责人 | Inspection/Test | BR-006 |
| SR-049 | 约束 | P1 | SYS-I |  | 硬件/QA | 硬件负责人 | Inspection/Test | BR-006 |
| SR-050 | 约束 | P1 | SYS-I |  | 硬件/QA | 硬件负责人 | Inspection/Test | BR-006 |
| SR-051 | 约束 | P1 | SYS-C | SYS-I | 硬件+UI | 硬件负责人 | Test | BR-006 |
| SR-052 | 约束 | P2 | SYS-H | SYS-E/D | 产测+固件 | 产测负责人 | Inspection/Test | BR-005 |
| SR-053 | 运维 | P1 | SYS-D | SYS-E | STM32+nRF | 存储/固件负责人 | Test | BR-005 |
| SR-054 | 运维 | P1 | SYS-G | SYS-F/E | App + BLE | App负责人 | Test | BR-005 |
| SR-055 | 运维 | P2 | SYS-G | SYS-F | App/平台可选 | App/后端负责人 | Inspection | BR-005 |
| SR-056 | 运维 | P2 | SYS-H | SYS-D | 产测+存储 | 产测负责人 | Inspection | BR-005 |
| SR-057 | 功能 | P0 | SYS-C |  | STM32 UI | UI负责人 | Test | BR-001 |
| SR-058 | 功能 | P1 | SYS-C |  | STM32 UI | UI负责人 | Test | BR-001 |
| SR-059 | 功能 | P1 | SYS-C | SYS-B | STM32 UI | UI负责人 | Test | BR-002 |
| SR-060 | 功能 | P2 | SYS-C | SYS-E/A | STM32 UI | UI负责人 | Test | BR-001 |
| SR-061 | 功能 | P0 | SYS-D | SYS-B/E/F | STM32存储 | 存储负责人 | Test | BR-002/005 |
| SR-062 | 约束 | P1 | SYS-D |  | STM32存储 | 存储负责人 | Inspection/Test | BR-005 |
| SR-063 | 功能 | P1 | SYS-D | SYS-B | STM32存储 | 存储负责人 | Test | BR-001 |
| SR-064 | 约束 | P2 | SYS-D | SYS-G | 存储+App解析 | 存储负责人 | Inspection | BR-005 |
| SR-065 | 可靠性 | P0 | SYS-E |  | STM32+nRF | BLE负责人 | Test | BR-002 |
| SR-066 | 可靠性 | P1 | SYS-E | SYS-A | STM32+nRF | BLE负责人 | Test | BR-005 |
| SR-067 | 可靠性 | P1 | SYS-E |  | STM32+nRF | BLE负责人 | Test | BR-002 |
| SR-068 | 架构 | P2 | SYS-E |  | STM32+nRF | BLE负责人 | Inspection | BR-006 |
| SR-069 | 流程 | P1 | SYS-H | SYS-Z | 测试体系 | QA负责人 | Inspection | BR-005 |
| SR-070 | 运维 | P1 | SYS-D | SYS-I | STM32固件 | 存储负责人 | Test | BR-005 |
| SR-071 | 运维 | P2 | SYS-D | SYS-H | 工具链 | QA负责人 | Demo | BR-005 |
| SR-072 | 运维 | P2 | SYS-H | SYS-I | 文档+流程 | QA负责人 | Inspection | BR-005 |
| SR-073 | 流程 | P2 | SYS-Z | SYS-H | 研发流程 | 系统工程师 | Inspection | BR-005 |

## 7. 基线、变更与评审（Baseline & Change Control）

### 7.1 RAM 基线建议

RAM 与 SyRS 一起形成需求基线（Requirement Baseline）。建议在Git仓库中用Tag与文档版本对齐，例如：req-baseline-v1.0。

### 7.2 变更规则（CR）

任何 SR 的新增/删除/修改必须提交变更单（CR）说明原因与影响；更新 SyRS 与 RAM；若涉及关键决策，新增或更新 ADR（ADR-xxx）。

## 8. 交付物（Outputs / Deliverables）

- RAM（本文）：系统需求到工程落点的分配。
- SID：所有跨域接口的清单与契约。
- ADR：关键架构选择与取舍记录。
- Verification Plan & Trace：SR→TC→证据闭环（由QA牵头）。
