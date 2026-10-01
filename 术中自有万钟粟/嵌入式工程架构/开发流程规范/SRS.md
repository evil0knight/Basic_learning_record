# 系统需求规格说明书（SRS）

[← 开发流程规范](./MOC.md) | [← 主页](../../../index.md)

> 来源：[Stage5 02 系统需求规格说明书 Software Requirements Specification（SRS）](https://twd6onxsxva.feishu.cn/docx/Clu8dkdjwoODIexxxMXcpX04n4e)

---

| 项目/产品 | 生命体征监控智能手表（工业/矿井/海上平台等受限网络场景） |
| --- | --- |
| 文档名称 | Software Requirements Specification (SRS) |
| 版本 | V1.0 |
| 状态 | Draft / Internal Review |
| 编写 | 课程研发组（模拟大厂流程） |
| 日期 | 2025-12-29 |
| 适用范围 | 主控MCU（STM32F411）固件 + 与BLE协处理器（nRF52840）的接口软件 |

## 1. 文档概述

### 1.1 目的

本文档用于定义智能手表“软件层面”必须实现的功能、接口、约束与非功能指标，作为软件设计（SDD）、实现（Implementation）与验证（Verification）的依据，并与系统级需求（SyRS）保持可追踪。

### 1.2 范围

本SRS覆盖：主控MCU固件（UI、传感采集、数据存储、功耗管理、与BLE协处理器通信、OTA/日志等）及其对外接口。不覆盖：硬件原理图/PCB/BOM细节（见HRS/HW Spec），手机App与云平台的详细需求（见BRD/平台需求文档）。

### 1.3 术语与缩写

| 缩写 | 英文 | 中文解释 |
| --- | --- | --- |
| SN | Stakeholder Needs | 干系人需求：来自客户/法规/业务方的“为什么要做” |
| UND | User Needs Document | 用户需求文档：从用户视角描述“要解决什么问题/用起来什么体验” |
| BRD | Business Requirements Document | 业务需求：业务目标、范围、成功指标、成本/进度约束 |
| SyRS | System Requirements Specification | 系统需求：跨软硬件的系统能力与约束（系统级“shall”） |
| SAD | System Architecture Document | 系统架构：分层/模块/接口与关键决策 |
| RAM | Requirement Allocation Matrix | 需求分配矩阵：系统需求→软硬件/模块/责任人 |
| ADR | Architecture Decision Record | 架构决策记录：关键取舍、原因、影响与备选方案 |
| SSRD | Software System Requirements Definition | 软件系统需求定义：系统级软件范围的需求集合/高层软件需求（HLR） |
| SRS | Software Requirements Specification | 软件需求规格：面向实现与测试的可验证软件需求（HLR细化/部分LLR） |
| SDD | Software Design Description | 软件设计说明：架构与详细设计（模块/接口/数据结构/状态机等） |
| OTA | Over-The-Air Upgrade | 无线/远程升级（本项目包含BLE/有线下载等路径） |

### 1.4 参考文档

• Stage2 业务需求文档（BRD）_重构版

• Stage3 系统需求文档（SyRS）_重构版

• Stage4 系统架构文档（SAD）_重构版 / RAM / ADR / SID

• Stage5 软件系统需求定义（SSRD）_重构版

## 2. 产品与运行环境概览

### 2.1 使用场景与核心痛点

目标环境包括：矿井、山区、海上作业平台、野外探险等“蜂窝网络覆盖差/不可用”场景。核心痛点：无法依赖手机网络完成持续在线；现场需要“生命体征监测 + 本地告警 + 近距离通信/组网 + 可追溯日志”。

### 2.2 用户与干系人

• 一线作业人员（佩戴者）

• 班组长/安全员（现场管理）

• 企业EHS/信息化部门（平台对接、合规）

• 研发/测试/运维（交付与维护）

### 2.3 系统边界与外部系统

手表本体通过BLE（nRF52840）与手机/网关进行近距离通信；GPS定位不在手表端直接采集，默认通过手机侧定位结果经BLE同步到手表（本版本不包含NFC）。

图2-1 系统与硬件资源示意（示例）

## 3. 软件总体说明

### 3.1 软件分层与模块

建议软件采用“驱动层（Driver/BSP）— 服务层（Service）— 应用层（App）”的分层架构：

• Driver/BSP：外设驱动、芯片驱动（LCD/TP/Flash/Sensor/RTC等），对上提供稳定接口。

• Service：将通用能力做成服务（UI服务、传感服务、存储服务、通信服务、OTA服务、日志与诊断服务），隔离硬件差异与并发细节。

• App：面向业务的场景流程（监测、告警、联动、设置、数据导出），仅调用服务接口。

### 3.2 运行时与任务模型（建议）

主控MCU建议使用FreeRTOS（或等价RTOS）。典型任务划分：

• UI_Task：LVGL刷新与输入事件处理

• Sensor_Task：周期采样/滤波/阈值判断

• Comms_Task：与nRF串口协议、数据同步

• Storage_Task：日志落盘、数据归档

• Ota_Task：升级流程与安全校验

并通过消息队列/事件组/互斥量实现线程安全与解耦。

### 3.3 数据与日志

本项目需要同时支持：

• 运行日志（Debug/Info/Warn/Error）用于定位问题

• 关键业务数据（生命体征/告警/定位快照）用于追溯

数据默认保存在外部SPI Flash；支持按策略滚动覆盖与导出。

## 4. 外部接口需求

### 4.1 硬件接口

| 模块 | 器件/型号 | 接口 | 关键说明 |
| --- | --- | --- | --- |
| LCD | ST7789V | SPI | 240×280，16-bit RGB565，≥30Hz刷新 |
| TP触控 | 电容触控IC | I2C + INT | 支持中断触发坐标上报 |
| 温湿度 | AHT21 | I2C | 体感/环境温度（±0.2℃目标） |
| PPG心率 | PPG Sensor | I2C/SPI | 心率误差目标±3BPM（系统级） |
| 加速度计 | MPU6050 | I2C + INT | 运动/跌倒检测输入 |
| 外部Flash | W25Qxx | SPI/QSPI | 日志/数据/升级包缓存 |
| BLE协处理器 | nRF52840 | UART + GPIO | BLE通信、可选Mesh；与主控协议见4.2 |
| 电源管理 | PMIC/充电管理 | GPIO/ADC/I2C | 电池电量采样、充电状态检测 |

### 4.2 软件接口

• 主控↔BLE协处理器：串口协议（帧头/长度/命令/序号/CRC），支持重传与超时；

• BLE GATT服务（建议）：设备信息、实时数据、告警事件、日志摘要、定位同步；

• 与PC工具：可选USB CDC/串口（用于调试与有线升级）。

### 4.3 人机交互

• LCD显示 + 电容触控

• 物理按键（可选）用于唤醒/确认/紧急呼救

• 蜂鸣器/振动马达（可选）用于告警反馈

## 5. 功能性需求

说明：本章需求采用“shall”表述，必须可验证（Test/Analysis/Inspection/Demo）。

| 需求ID | 需求名称 | 需求描述（shall） | 优先级 | 追溯来源 | 验证方式 |
| --- | --- | --- | --- | --- | --- |
| SRS-FR-UI-001 | 基础表盘 | 系统应提供默认表盘界面，显示时间、日期与电量状态。 | Must | SyRS-FUNC-UI | Test/Demo |
| SRS-FR-UI-002 | 多页面导航 | 系统应支持从表盘进入健康、告警、设置、日志等页面，并支持返回/主页操作。 | Must | UND-UX | Test/Demo |
| SRS-FR-UI-003 | 触控事件 | 系统应支持触控点击/滑动等基本手势，并将触控事件在50ms内投递到UI逻辑。 | Should | SyRS-NFR-RESP | Test |
| SRS-FR-UI-004 | 亮度控制 | 系统应支持背光亮度分级（≥5档）与自动息屏策略。 | Must | SyRS-FUNC-PWR | Test |
| SRS-FR-UI-005 | 告警弹窗 | 当出现生命体征异常/跌倒/紧急呼救时，系统应在UI上弹出高优先级告警页并提供确认/解除入口。 | Must | SyRS-FUNC-ALM | Test/Demo |
| SRS-FR-SEN-001 | 温度采集 | 系统应以可配置周期（默认1s~10s）采集温度数据，并提供最近N点缓存用于滤波。 | Must | SyRS-FUNC-SEN | Test |
| SRS-FR-SEN-002 | 心率采集 | 系统应支持心率数据的周期采集与有效性判断（如佩戴检测/信号质量）。 | Must | SyRS-FUNC-SEN | Test |
| SRS-FR-SEN-003 | 运动/跌倒输入 | 系统应接收加速度计中断或轮询数据，并支持跌倒检测算法的输入数据通道。 | Should | SyRS-FUNC-SAF | Test |
| SRS-FR-SEN-004 | 阈值告警 | 系统应支持对心率/体温等指标设置阈值，并在超阈值持续T秒（可配置）后触发告警事件。 | Must | SyRS-FUNC-ALM | Test |
| SRS-FR-SEN-005 | 传感器自检 | 系统应在上电后对关键传感器进行通信自检（I2C/SPI），失败时上报故障码并降级运行。 | Must | SyRS-FUNC-DIAG | Test/Inspection |
| SRS-FR-DATA-001 | 本地日志 | 系统应支持运行日志记录（等级、时间戳、模块、事件码），并可配置滚动策略。 | Must | SyRS-FUNC-LOG | Test |
| SRS-FR-DATA-002 | 业务数据存储 | 系统应存储生命体征采样摘要、告警事件与定位快照，并支持按时间查询。 | Should | BRD-KPI | Test |
| SRS-FR-DATA-003 | 数据导出 | 系统应支持通过BLE或有线方式导出日志与业务数据（导出格式可配置：CSV/二进制）。 | Should | SyRS-FUNC-EXP | Test/Demo |
| SRS-FR-DATA-004 | 断电保护 | 系统应对关键索引/元数据进行掉电保护，保证异常掉电后不导致Flash文件系统不可恢复。 | Must | SyRS-NFR-REL | Test/Analysis |
| SRS-FR-COM-001 | 主控↔BLE协议 | 主控与nRF通信应采用带序号与CRC的帧协议，并支持超时重传（默认≤3次）。 | Must | SyRS-FUNC-COM | Test |
| SRS-FR-COM-002 | 定位同步 | 系统应支持接收手机端GPS定位（经BLE传入），并在告警事件中关联最近一次定位信息。 | Should | BRD-Safety | Test/Demo |
| SRS-FR-COM-003 | 时间同步 | 系统应支持通过手机/网关同步时间，并保证时间戳单调性（避免回拨造成日志错乱）。 | Must | SyRS-FUNC-TIME | Test |
| SRS-FR-COM-004 | 离线工作 | 在无手机/网关连接时，系统应保持本地监测与告警能力，并在恢复连接后补传事件。 | Must | SyRS-FUNC-OFF | Test/Demo |
| SRS-FR-OTA-001 | 升级触发 | 系统应支持从BLE/有线通道接收升级包并进入升级流程（用户确认或策略触发）。 | Must | SyRS-FUNC-OTA | Test |
| SRS-FR-OTA-002 | 完整性校验 | 升级包在写入前与写入后均应进行完整性校验（CRC32/MD5等算法由SSRD定义）。 | Must | SyRS-FUNC-OTA | Test |
| SRS-FR-OTA-003 | 安全升级 | 系统应支持升级包加密/签名校验（算法与密钥管理见安全需求），防止篡改包刷入。 | Must | SyRS-FUNC-SEC | Test/Inspection |
| SRS-FR-OTA-004 | 失败回滚 | 升级失败（校验失败/断电/超时）时系统应保持可启动并回到旧版本。 | Must | SyRS-NFR-REL | Test |
| SRS-FR-PWR-001 | 工作模式 | 系统应支持至少三种工作模式：正常、低功耗待机、关机/超低功耗；并定义各模式唤醒源。 | Must | SyRS-FUNC-PWR | Test |
| SRS-FR-PWR-002 | 电量采样 | 系统应通过ADC/I2C等方式采样电池电压并估算电量百分比，采样周期可配置。 | Must | SyRS-FUNC-PWR | Test |
| SRS-FR-PWR-003 | 低电告警 | 当电量低于阈值时系统应触发低电提示与省电策略（降低亮度/降低采样频率等）。 | Should | UND-UX | Test |
| SRS-FR-SEC-001 | 权限与配对 | 系统应支持与手机/网关的绑定关系管理（首次绑定、自动重连、解绑/重置）。 | Should | SyRS-FUNC-SEC | Test/Demo |
| SRS-FR-DIAG-001 | 故障码体系 | 系统应提供统一故障码（模块+原因）并可在UI/日志/导出中体现。 | Must | SyRS-FUNC-DIAG | Inspection/Test |
| SRS-FR-DIAG-002 | 看门狗策略 | 系统应配置独立看门狗，并定义喂狗策略、超时处置与重启后自恢复流程。 | Must | SyRS-NFR-REL | Test |

## 6. 非功能性需求（NFR）

| 需求ID | 类别 | 需求描述（shall） | 目标/阈值 | 验证方式 |
| --- | --- | --- | --- | --- |
| SRS-NFR-PERF-001 | 响应 | UI触控到界面反馈的平均延迟应≤100ms，95分位≤200ms。 | ≤100/200ms | Test |
| SRS-NFR-PERF-002 | 采样吞吐 | 在默认采样配置下，传感采集与UI刷新不应互相阻塞导致丢采样（丢样率≤0.1%）。 | ≤0.1% | Test/Analysis |
| SRS-NFR-PWR-001 | 续航 | 在典型使用场景下（默认采样+每天N次告警/同步），续航目标≥X天（由产品阶段定义）。 | ≥X天 | Test |
| SRS-NFR-REL-001 | 稳定性 | 系统应支持连续运行≥72小时无死机（除看门狗复位外）。 | ≥72h | Test |
| SRS-NFR-REL-002 | 掉电恢复 | 异常掉电后，系统应在30秒内恢复到可工作状态，并保证关键数据一致性。 | ≤30s | Test |
| SRS-NFR-SEC-001 | 升级安全 | 升级包应具备防篡改能力；未通过校验的包不得写入生效分区。 | 0次绕过 | Test/Inspection |
| SRS-NFR-MAINT-001 | 可维护 | 日志应可定位到模块与事件码；关键流程应具备统计计数（失败次数、重传次数等）。 | 可追溯 | Inspection |
| SRS-NFR-PORT-001 | 可移植 | Driver/BSP层与Service/App层应通过接口解耦，便于未来替换屏幕/传感器型号。 | 接口稳定 | Review/Inspection |

## 7. 约束、标准与合规

• 编码规范：建议遵循MISRA C:2012（关键模块），并建立静态分析规则集。

• 资源约束：STM32F411（128KB RAM/512KB Flash）为主控资源基线；需要在SyRS/SSRD中给出内存预算与CPU预算。

• 第三方组件：LVGL v8.3、FatFS（如使用）等需记录版本与修改点。

• 安全合规：密钥与标志位建议存储在安全元件/加密芯片（含EEPROM）中；算法与密钥生命周期需在SSRD/安全章节定义。

## 8. 验证计划与可追踪性

### 8.1 验证层级

• 单元测试：Driver/Service关键模块（可在PC仿真或HIL）

• 集成测试：任务间交互、协议栈、存储与OTA

• 系统测试：场景化验证（离线监测、告警、恢复连接补传、升级回滚）

• 现场测试：受限网络环境/电磁干扰/温度等

### 8.2 需求追踪矩阵（示例）

| SyRS需求 | 对应SRS需求 | 测试用例 | 备注 |
| --- | --- | --- | --- |
| SyRS-FUNC-OTA | SRS-FR-OTA-001~004 | TC-OTA-01~05 | 升级触发/校验/回滚 |
| SyRS-FUNC-ALM | SRS-FR-UI-005, SRS-FR-SEN-004 | TC-ALM-01~04 | 阈值告警/弹窗/记录 |
| SyRS-FUNC-COM | SRS-FR-COM-001~004 | TC-COM-01~06 | 协议、离线补传、同步 |

## 9. 附录

### 9.1 需求编写规则

• 使用“系统应/shall”描述，避免实现细节；

• 每条需求必须可验证，并给出验证方式；

• 需求必须具备来源（UND/BRD/SyRS）与优先级（Must/Should/Could）。

### 9.2 变更与基线（占位）

SRS的版本基线建议与Git Tag/Release基线绑定：

• Tag: srs-v1.0 对应SRS V1.0

• 每次合并到release分支需更新需求追踪与变更记录（详细策略见Stage7配置管理文档）。
