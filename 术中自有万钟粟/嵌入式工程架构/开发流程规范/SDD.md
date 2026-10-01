# 软件设计说明书（SDD）

[← 开发流程规范](./MOC.md) | [← 主页](../../../index.md)

> [参考,仅复制](https://twd6onxsxva.feishu.cn/docx/Y8j7dSk1xoVfCwxsvHrcnzKlnLb)

---

> 原文标题：Stage6 01 软件设计说明书 Software Design Description（SDD）

| 产品/项目名称 | EC‑S100 受限网络工况下生命体征监控智能手表（STM32F411 + nRF52840 双 MCU） |
| --- | --- |
| 文档类型 | Software Design Description（SDD） |
| 阶段定位 | 承接 SRS/SSRD，落地到“模块/任务/接口/数据/异常/验证”的可实施设计 |
| 目标读者 | 嵌入式工程师、软件架构师、测试/QA、硬件工程师（接口协同）、项目经理 |
| 版本/状态 | V1.0 / Draft（可评审） |
| 编写日期 | 2025-12-30 |

## 文档变更记录

| 版本 | 日期 | 修订内容 | 修订人 | 审核人 |
| --- | --- | --- | --- | --- |
| V1.0 | 2025-12-30 | 基于现有 Stage6 SDD 初稿与 EC‑S100 软件架构设计文档重构完善：补全分层、接口、任务、数据、低功耗、安全与可验证性内容。 | Liu Yueqi | TBD |

## 1 引言

本文档描述 EC‑S100 智能手表在 STM32F411 侧的软件总体方案与详细设计。设计以“受限网络工况”为核心约束：现场弱网/无网、环境恶劣、人员安全优先，系统需具备本地闭环能力（采集‑处理‑告警‑存储），并在可用链路出现时完成数据同步与事件上报。

本 SDD 重点覆盖：软件分层与模块职责、运行时任务与并发、关键接口与协议、数据与存储模型、低功耗与可靠性、安全与 OTA、以及与 SRS/SSRD 的设计追踪。

### 1.1 范围

范围覆盖 STM32F411 侧：显示与交互、传感器采集与预处理、本地存储与日志、与 nRF52840 的串口通信、低功耗与电源管理、异常诊断与恢复。nRF52840 侧 BLE/mesh 仅在接口层描述其交互契约。

### 1.2 参考与输入

输入来源：Stage5 SSRD/SRS、Stage4 SAD/SID/RAM/ADR、以及《EC‑S100 智能手表软件架构设计文档（STM32F411 侧）》中的目录结构与核心机制（工厂实例化、模块划分、任务/事件/消息约定等）。

### 1.3 术语与缩写

| 缩写 | 含义 |
| --- | --- |
| SDD | Software Design Description / 软件设计说明书 |
| SyRS | System Requirements Specification / 系统需求规格说明书 |
| SSRD | Software System Requirements Definition / 软件系统需求定义 |
| SRS | Software Requirements Specification / 软件需求规格说明书 |
| SAD | System Architecture Document / 系统架构文档 |
| SID | System Interface Definition / 系统接口定义 |
| RAM | Requirement Allocation Matrix / 需求分配矩阵 |
| ADR | Architecture Decision Record / 架构决策记录 |
| PMIC | Power Management IC / 电源管理芯片 |
| HIL | Hardware‑in‑the‑Loop / 硬件在环测试 |

## 2 业务与场景约束（受限网络工况）

### 2.1 典型场景

典型使用场景包括：矿井/隧道/山区/海上平台等蜂窝网络覆盖差或不可用区域；作业人员需要实时生命体征监控与本地告警，并在有链路时将关键信息同步给手机/网关/平台。GPS 定位由手机侧提供（BLE 获取定位信息），手表侧不集成独立 GPS。

### 2.2 设计目标（面向市场现实）

| 目标 | 工程解释（为什么这么做） | 主要落点 |
| --- | --- | --- |
| 本地闭环安全 | 弱网条件下无法依赖云端；告警必须在设备侧完成。 | 阈值/规则引擎、本地弹窗/蜂鸣/震动、离线事件队列 |
| 低功耗长续航 | 户外/作业场景充电不便；必须可控功耗与可预期续航。 | 状态机、Tickless/Stop、分域上电、采样占空比 |
| 鲁棒与可恢复 | 恶劣环境 + 长时间运行；故障必须可诊断且自动恢复。 | 看门狗、断言/回溯、掉电保护、重试/超时、日志 |
| 可演进与平台化 | 后续要换屏/换传感器/换 MCU；软件要能平移。 | 分层架构、驱动抽象、工厂实例化、接口契约、配置化 |

## 3 总体软件架构

### 3.1 部署视图（双 MCU 分工）

系统采用 STM32F411 + nRF52840 双 MCU：

| 处理器 | 主要职责 | 关键接口 |
| --- | --- | --- |
| STM32F411（主控/UI） | 显示与交互、传感器采集与预处理、本地存储/日志、低功耗控制、与 BLE 协处理通信 | SPI/I2C/ADC/RTC/UART、外部 Flash |
| nRF52840（BLE 协处理） | BLE 5.x/mesh、与手机/网关连接、定位信息转发、与 STM32 串口协议交互 | UART（与 STM32）、BLE/GATT/mesh |

两者通过 UART 进行协议化通信，采用“请求‑应答 + 超时重传 + CRC 校验”的可靠传输策略，避免现场强干扰导致的 silent failure。

图 3‑1 双 MCU 部署与外设连接示意（工程资料）

### 3.2 分层架构（BSP/Driver/Service/App）

STM32F411 侧采用四层分层：BSP → Driver → Service → App。其核心价值是隔离变化：硬件变化影响 BSP/Driver；业务变化主要在 App；跨模块协同放在 Service，避免 App 之间强耦合。

| 层级 | 职责边界 | 典型模块（EC‑S100） |
| --- | --- | --- |
| BSP（板级支持） | 时钟/中断/外设基础能力封装，提供稳定 HAL 适配层 | GPIO/I2C/SPI/UART/ADC/RTC/DMA/低功耗入口 |
| Driver（器件驱动） | 针对具体芯片/器件的数据手册实现读写与初始化，不关心业务 | ST7789 显示、触控控制器、W25Qxx Flash、MPU6050、温湿度/PPG 等 |
| Service（系统服务） | 跨驱动/跨应用的系统能力：调度、缓存、协议、存储、功耗、诊断 | DisplaySvc/SensorSvc/StorageSvc/BleBridgeSvc/PowerSvc/LogSvc/OtaSvc |
| App（业务应用） | 面向用户的功能组合与流程控制，调用服务层能力完成业务 | 表盘/健康/告警/设置/同步 等 |

### 3.3 工程目录与实例化机制（EC‑S100 真实工程约束）

EC‑S100 工程采用“工厂实例化（*_inst()）+ 句柄（handle）+ 接口表（function pointers）”的 C 语言 OOP 实现，支持按配置切换不同驱动实现。以下目录结构节选自现有工程文档：

| C<br>/* 系统事件类型 */<br>typedef enum {<br>    SYS_EVENT_NONE = 0,<br>    SYS_EVENT_SENSOR_UPDATE,<br>    SYS_EVENT_BATTERY_UPDATE,<br>    SYS_EVENT_BT_STATUS_CHANGE,<br>    SYS_EVENT_UI_SWITCH,<br>    SYS_EVENT_POWER_MODE_CHANGE<br>} system_event_type_t;<br><br>/* 界面状态枚举 */<br>typedef enum {<br>    UI_STATE_WATCHFACE = 0,<br>    UI_STATE_HEALTH,<br>    UI_STATE_SPORT,<br>    UI_STATE_SETTINGS,<br>    UI_STATE_WEATHER,<br>    UI_STATE_SLEEP<br>} ui_state_t;<br><br>/* 传感器数据请求结构 */<br>typedef struct {<br>    uint32_t sensor_mask;     /* 请求的传感器位掩码 */<br>    uint32_t sample_rate;     /* 采样频率 */<br>    uint32_t duration;        /* 采样持续时间 */<br>} sensor_request_t;<br><br>/* 系统状态全局变量 */<br>typedef struct {<br>    ui_state_t current_ui_state;<br>    uint8_t battery_level;<br>    uint8_t bt_status;<br>    uint8_t power_mode;<br>    uint32_t system_tick;<br>} system_status_t; |
| --- |

关键约定：

• 每个驱动提供 Xxx_inst() 统一构造入口；BSP 层通过配置注入 I2C/SPI/UART 适配函数指针。

• Service 层持有 Driver Instance，面向 App 暴露“稳定、可测试”的业务接口。

• App 不直接触达底层外设，避免并发访问与耦合扩散。

## 4 运行时设计（任务/事件/并发）

### 4.1 任务划分与优先级建议

| 任务 | 职责 | 依赖 | 优先级 | 触发/周期 |
| --- | --- | --- | --- | --- |
| UI_Task | LVGL 刷新/触控处理/页面路由 | DisplaySvc, TouchDrv | 高 | 20ms 周期刷新 / 事件触发 |
| Sensor_Task | 传感器采样、滤波、异常检测、事件发布 | SensorSvc, SensorDrv | 中高 | 10~100ms（按传感器配置） |
| BleBridge_Task | 与 nRF 串口通信、协议解析、重传/应答 | BleBridgeSvc, UART | 中 | 事件驱动 + 超时定时器 |
| Storage_Task | 日志/数据落盘、磨损均衡、导出 | StorageSvc, W25Qxx | 中 | 批量落盘（4KB 或 200ms） |
| Power_Task | 功耗状态机、唤醒源管理、电量估算 | PowerSvc, PMIC/ADC/RTC | 中 | 1s 心跳 + 事件触发 |
| Diag_Task | 健康监测、看门狗喂狗、错误上报 | LogSvc, WDT | 低 | 1s 心跳 |

说明：UI 与 Sensor 常为实时关键路径；Storage 采用批量策略避免擦写抖动影响实时性；BleBridge 以协议状态机保证可靠。

### 4.2 事件总线（发布‑订阅）

为降低 App 与 Service 的耦合，系统引入“事件总线（Event Bus）”：SensorSvc/PowerSvc/BleBridgeSvc 发布事件；App/UI 订阅处理。事件数据通过队列传递，支持去抖/合并与优先级。

| // 事件定义（示例）<br>typedef enum {<br>  EVT_HEART_RATE_UPDATE,<br>  EVT_SPO2_UPDATE,<br>  EVT_MOTION_FALL_DETECTED,<br>  EVT_BLE_CONNECTED,<br>  EVT_BLE_GPS_UPDATE,<br>  EVT_BATTERY_LOW,<br>  EVT_ALARM_TRIGGERED,<br>} app_event_id_t;<br><br>typedef struct {<br>  app_event_id_t id;<br>  uint32_t ts_ms;<br>  union {<br>    struct { uint16_t bpm; } hr;<br>    struct { uint8_t spo2; } spo2;<br>    struct { float lat, lon; uint32_t fix_ts; } gps;<br>    struct { uint8_t level; } batt;<br>  } u;<br>} app_event_t; |
| --- |

事件队列丢弃策略：安全相关（跌倒/求救/低电）优先保留；非关键更新（步数/环境温湿度）可覆盖。

### 4.3 资源并发与互斥策略

| 资源 | 冲突来源 | 策略 | 理由 |
| --- | --- | --- | --- |
| I2C 总线 | 多传感器 + 触控控制器 | 总线互斥（mutex）+ 统一 I2C Service 排队 | 避免并发访问导致 NACK/死锁 |
| SPI LCD/Flash | UI 刷新与 Flash 访问并发 | SPI 总线仲裁；LCD DMA 优先 | 保证 UI 帧率与体验 |
| 外部 Flash | 日志/数据/配置同时读写 | RAM 缓冲聚合写；后台磨损均衡 | 降低擦写抖动，提升寿命 |
| UART（nRF） | 上报与命令交织 | 协议层序列化；帧队列 + ACK 重传 | 保证可靠传输与可追踪 |

## 5 模块详细设计

### 5.1 DisplaySvc（显示服务）

职责：统一管理 LVGL、屏幕刷新策略、背光、界面路由与弹窗。对上提供 UI API；对下依赖 ST7789 Driver 与 Touch Driver。

| 子模块 | 职责 | 关键接口 |
| --- | --- | --- |
| LVGL Port | tick、flush、input driver 适配 | lv_port_disp_init(), lv_port_indev_init() |
| Screen Driver | SPI/DMA 刷屏，区域更新 | st7789_init(), st7789_flush(area, color_p) |
| Backlight | PWM/IO 控制，亮度曲线 | bl_set_level(level), bl_sleep() |
| UI Router | 页面栈与事件分发 | ui_push(page), ui_on_event(evt) |

刷新策略：默认 20ms tick；在静止/灭屏时降频或停止刷新，结合 PowerSvc 进入低功耗。

### 5.2 SensorSvc（传感器服务）

职责：抽象多个传感器的采集、校准、滤波、异常检测与数据发布。通过配置表决定采样周期与启停。

| 传感器 | 接口 | 数据输出 | 备注 |
| --- | --- | --- | --- |
| PPG/心率/血氧 | I2C/SPI（按具体器件） | bpm, spo2, signal_quality | 含佩戴检测与抗干扰 |
| 三轴加速度 MPU6050 | I2C + INT | acc/gyro, step, fall | 中断触发 + 滑窗 |
| 温湿度 | I2C | temp, hum | 低频采集 |
| 光照/电量 | ADC | lux, vbat | 用于背光/续航估算 |

关键算法位点：滤波（中值/滑动平均/简单卡尔曼）、跌倒（阈值+姿态变化+再确认计时器）、佩戴检测（信号质量组合判定）。

### 5.3 BleBridgeSvc（BLE 桥接服务，UART⇄nRF）

职责：定义 STM32 与 nRF 的消息协议、会话状态机、命令/上报队列、超时与重传。

| 帧格式（建议）：<br>\| SOF(0xA5) \| VER \| TYPE \| LEN \| PAYLOAD ... \| CRC16 \|<br>TYPE:<br>  0x01 上报：生命体征/告警/日志摘要<br>  0x02 命令：时间同步/参数下发/屏幕控制<br>  0x03 响应：ACK/NACK/错误码<br>超时重传：T=200ms，最多 3 次；连续失败进入 Link-Degraded 状态并上报。 |
| --- |

### 5.4 StorageSvc（存储服务）

职责：管理外部 Flash 分区、文件/记录格式、磨损均衡、数据导出与崩溃恢复。

| 分区 | 用途 | 建议大小 | 写入特性 |
| --- | --- | --- | --- |
| CFG | 配置参数/校准数据 | 64KB | 低频写，带版本与 CRC |
| LOG | 运行日志/错误码 | 512KB~2MB | 追加写 + 周期压缩 |
| DATA | 生命体征与事件记录 | 2MB~8MB | 循环缓冲（ring） |
| OTA | 升级镜像缓存 | 按镜像大小 | 块写入 + 校验 |

写入策略：4KB 聚合写/或 200ms 触发写入；掉电保护采用“双页提交（A/B header）+ CRC”。

### 5.5 PowerSvc（功耗与电源服务）

职责：管理功耗状态机（Active/Idle/ScreenOff/Stop）、唤醒源（RTC/按键/传感器中断/串口）、电量估算与低电策略。

| 状态 | 进入条件 | 退出/唤醒 | 关键动作 |
| --- | --- | --- | --- |
| Active | 有交互/告警/同步 | — | 全速运行，UI 20ms |
| Idle | 无交互，低频采集 | 触控/事件 | 降低 UI 刷新，传感器降占空 |
| ScreenOff | 灭屏 | 按键/抬腕/告警 | 关闭背光，停止 LVGL 刷新 |
| Stop(Tickless) | 长时间静止且无关键任务 | RTC/按键/中断 | 进入 Stop，外设按需下电 |

### 5.6 LogSvc（日志与诊断服务）

职责：统一日志 API（等级/模块/时间戳），支持串口输出、Flash 落盘、异常回溯（HardFault/Assert）、以及对外导出。

### 5.7 OtaSvc（升级与安全）

职责：升级镜像接收/校验/切换；与 nRF/手机协作完成传输；支持断点续传与完整性验证。升级包建议：完整性校验（CRC32/Hash）+ 可选 AES 加密 + 版本回滚保护。

## 6 接口设计

### 6.1 外部接口（对硬件/对协处理器）

| 接口对象 | 物理接口 | 协议/驱动 | 关键约束 |
| --- | --- | --- | --- |
| LCD ST7789 | SPI + GPIO | ST7789 Driver | 刷新带宽/帧率；DMA 优先 |
| 触控 | I2C + INT | Touch Driver | 中断去抖；与传感器共享 I2C |
| 外部 Flash W25Qxx | SPI | W25Qxx Driver | 擦写耗时；磨损均衡 |
| 传感器簇 | I2C/ADC/INT | Sensor Drivers | 采样周期/中断优先级 |
| nRF52840 | UART | BleBridge Protocol | ACK/重传；帧校验；流控 |
| PMIC/电池 | ADC/GPIO | PowerSvc | 低电/充电状态变化 |

### 6.2 内部接口（Service⇄App）

| // SensorSvc<br>int sensor_get_latest(sensor_id_t id, sensor_sample_t* out);<br>int sensor_set_rate(sensor_id_t id, uint16_t hz);<br>int sensor_subscribe(app_event_id_t evt, event_cb_t cb);<br><br>// StorageSvc<br>int storage_append(record_type_t type, const void* buf, uint16_t len);<br>int storage_export(export_sink_t sink, export_filter_t* filter);<br><br>// PowerSvc<br>power_state_t power_get_state(void);<br>int power_request_state(power_state_t target, uint32_t timeout_ms); |
| --- |

## 7 数据设计（数据模型/格式/一致性）

生命体征与事件采用“定长头 + 可变 payload”的二进制记录，包含：类型、长度、时间戳、序列号、CRC。无网络时以本地 RTC 为准；连上手机后进行时间校正并记录校正偏差。配置采用版本化结构体（magic/version/size/CRC）。

## 8 可靠性与异常处理

看门狗由 Diag_Task 喂狗；喂狗条件包含关键任务心跳、队列堵塞检测、Flash 擦写超时检测。异常触发软复位并写入崩溃上下文（原因码 + PC/LR + 栈回溯摘要）。错误码分层：模块域 + 子域 + 具体码，可导出/上报。

## 9 性能与资源预算（示例）

| 路径/资源 | 预算建议 | 说明 |
| --- | --- | --- |
| UI 刷新周期 | ≤ 20ms | 交互体验与功耗平衡 |
| UART 单帧处理 | ≤ 5ms | 避免阻塞 RTOS 调度 |
| 关键告警检测延迟 | ≤ 200ms | 跌倒/求救等安全路径 |
| Flash 写入 | 后台/分片 | 不可阻塞 UI/Sensor |
| 任务栈 | UI≥4KB, Sensor≥3KB, Storage≥3KB | 以 watermark 统计校准 |

## 10 可验证性设计（Verification‑Ready）

Service 层接口可 Mock 以做单元测试；HIL 覆盖：传感器波形回放、串口协议 Fuzz、Flash 擦写压力与掉电恢复、低功耗唤醒（RTC/按键/中断）测试。验收用例需与 SRS/SSRD 逐条映射。

## 11 需求‑设计追踪（Traceability）

每条 SRS/SSRD 需求至少对应一个设计落点（模块/接口/任务/数据结构/参数）并能落到验证项。

| 需求ID | 需求标题 | 设计落点 | 验证建议 |
| --- | --- | --- | --- |
| SRS-FR-UI-001 | 基础表盘 | DisplaySvc / UI_Task / TouchDrv / ST7789Drv | UI 用例 + 帧率/响应测试 |
| SRS-FR-UI-002 | 多页面导航 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-UI-003 | 触控事件 | DisplaySvc / UI_Task / TouchDrv / ST7789Drv | UI 用例 + 帧率/响应测试 |
| SRS-FR-UI-004 | 亮度控制 | DisplaySvc / UI_Task / TouchDrv / ST7789Drv | UI 用例 + 帧率/响应测试 |
| SRS-FR-UI-005 | 告警弹窗 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SRS-FR-SEN-001 | 温度采集 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-SEN-002 | 心率采集 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-SEN-003 | 运动/跌倒输入 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SRS-FR-SEN-004 | 阈值告警 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SRS-FR-SEN-005 | 传感器自检 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-DATA-001 | 本地日志 | StorageSvc / LogSvc / W25QxxDrv | 压力测试 + 掉电恢复测试 |
| SRS-FR-DATA-002 | 业务数据存储 | StorageSvc / LogSvc / W25QxxDrv | 压力测试 + 掉电恢复测试 |
| SRS-FR-DATA-003 | 数据导出 | StorageSvc / LogSvc / W25QxxDrv | 压力测试 + 掉电恢复测试 |
| SRS-FR-DATA-004 | 断电保护 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-COM-001 | 主控↔BLE协议 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SRS-FR-COM-002 | 定位同步 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SRS-FR-COM-003 | 时间同步 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SRS-FR-COM-004 | 离线工作 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-OTA-001 | 升级触发 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-OTA-002 | 完整性校验 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-01 | 生命体征采集 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-02 | 运动/姿态采集 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-03 | 数据预处理 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-04 | 本地阈值告警 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SSRD-F-05 | 一键求救 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SSRD-F-06 | 离线运行 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-07 | 实时显示 | DisplaySvc / UI_Task / TouchDrv / ST7789Drv | UI 用例 + 帧率/响应测试 |
| SSRD-F-08 | 交互与设置 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-09 | 数据存储 | StorageSvc / LogSvc / W25QxxDrv | 压力测试 + 掉电恢复测试 |
| SSRD-F-10 | 日志与诊断 | StorageSvc / LogSvc / W25QxxDrv | 压力测试 + 掉电恢复测试 |
| SSRD-F-11 | BLE连接与配对 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SSRD-F-12 | 数据同步 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SSRD-F-13 | 定位获取 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SSRD-F-14 | 远程上报协同 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-15 | OTA升级 | SensorSvc / App Logic | 功能用例 + 边界测试 |

注：完整追踪矩阵见附录 A。

## 附录 A：完整需求‑设计追踪矩阵（SRS + SSRD）

| 需求ID | 需求标题 | 设计落点 | 验证建议 |
| --- | --- | --- | --- |
| SRS-FR-UI-001 | 基础表盘 | DisplaySvc / UI_Task / TouchDrv / ST7789Drv | UI 用例 + 帧率/响应测试 |
| SRS-FR-UI-002 | 多页面导航 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-UI-003 | 触控事件 | DisplaySvc / UI_Task / TouchDrv / ST7789Drv | UI 用例 + 帧率/响应测试 |
| SRS-FR-UI-004 | 亮度控制 | DisplaySvc / UI_Task / TouchDrv / ST7789Drv | UI 用例 + 帧率/响应测试 |
| SRS-FR-UI-005 | 告警弹窗 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SRS-FR-SEN-001 | 温度采集 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-SEN-002 | 心率采集 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-SEN-003 | 运动/跌倒输入 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SRS-FR-SEN-004 | 阈值告警 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SRS-FR-SEN-005 | 传感器自检 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-DATA-001 | 本地日志 | StorageSvc / LogSvc / W25QxxDrv | 压力测试 + 掉电恢复测试 |
| SRS-FR-DATA-002 | 业务数据存储 | StorageSvc / LogSvc / W25QxxDrv | 压力测试 + 掉电恢复测试 |
| SRS-FR-DATA-003 | 数据导出 | StorageSvc / LogSvc / W25QxxDrv | 压力测试 + 掉电恢复测试 |
| SRS-FR-DATA-004 | 断电保护 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-COM-001 | 主控↔BLE协议 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SRS-FR-COM-002 | 定位同步 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SRS-FR-COM-003 | 时间同步 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SRS-FR-COM-004 | 离线工作 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-OTA-001 | 升级触发 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-OTA-002 | 完整性校验 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-OTA-003 | 安全升级 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-OTA-004 | 失败回滚 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-PWR-001 | 工作模式 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-PWR-002 | 电量采样 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-PWR-003 | 低电告警 | PowerSvc / Tickless / ADC Battery Estimation | 功耗曲线/唤醒测试 + 低电策略用例 |
| SRS-FR-SEC-001 | 权限与配对 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-DIAG-001 | 故障码体系 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-DIAG-002 | 看门狗策略 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-NFR-PERF-001 | 响应 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-NFR-PERF-002 | 采样吞吐 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-NFR-PWR-001 | 续航 | PowerSvc / Tickless / ADC Battery Estimation | 功耗曲线/唤醒测试 + 低电策略用例 |
| SRS-NFR-REL-001 | 稳定性 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-NFR-REL-002 | 掉电恢复 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-NFR-SEC-001 | 升级安全 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-NFR-MAINT-001 | 可维护 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-NFR-PORT-001 | 可移植 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-OTA-001~004 | SRS-FR-OTA-001~004 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-UI-005, | SRS-FR-UI-005, SRS-FR-SEN-004 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SRS-FR-COM-001~004 | SRS-FR-COM-001~004 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-01 | 生命体征采集 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-02 | 运动/姿态采集 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-03 | 数据预处理 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-04 | 本地阈值告警 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SSRD-F-05 | 一键求救 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SSRD-F-06 | 离线运行 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-07 | 实时显示 | DisplaySvc / UI_Task / TouchDrv / ST7789Drv | UI 用例 + 帧率/响应测试 |
| SSRD-F-08 | 交互与设置 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-09 | 数据存储 | StorageSvc / LogSvc / W25QxxDrv | 压力测试 + 掉电恢复测试 |
| SSRD-F-10 | 日志与诊断 | StorageSvc / LogSvc / W25QxxDrv | 压力测试 + 掉电恢复测试 |
| SSRD-F-11 | BLE连接与配对 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SSRD-F-12 | 数据同步 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SSRD-F-13 | 定位获取 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SSRD-F-14 | 远程上报协同 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-15 | OTA升级 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-16 | 安全机制 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-17 | 低功耗管理 | PowerSvc / Tickless / ADC Battery Estimation | 功耗曲线/唤醒测试 + 低电策略用例 |
| SSRD-F-18 | 异常恢复 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-D-01 | 数据模型 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-D-02 | 时间戳 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-D-03 | 数据保留策略 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-I-01 | MCU间通信协议 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-I-02 | BLE GATT接口 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SSRD-I-03 | 手机APP接口约束 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-I-04 | 存储接口抽象 | StorageSvc / LogSvc / W25QxxDrv | 压力测试 + 掉电恢复测试 |
| SSRD-I-05 | 传感器驱动抽象 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-NF-P-01 | 采集与告警延迟 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SSRD-NF-P-02 | 启动时间 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-NF-P-03 | 续航目标 | PowerSvc / Tickless / ADC Battery Estimation | 功耗曲线/唤醒测试 + 低电策略用例 |
| SSRD-NF-R-01 | 稳定性 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-NF-R-02 | 数据完整性 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-NF-S-01 | 通信安全 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-NF-M-01 | 可维护性 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-01, | 离线采集生命体征 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-04, | 异常触发告警 | SensorSvc + Rule Engine + DisplaySvc Popup + BleBridge Report | 场景复现/阈值回归 + 误报率统计 |
| SSRD-F-06, | 数据同步与补发 | BleBridgeSvc / UART Protocol / BleBridge_Task | 协议互通 + 丢包/重传测试 |
| SSRD-F-17, | 低功耗与续航 | PowerSvc / Tickless / ADC Battery Estimation | 功耗曲线/唤醒测试 + 低电策略用例 |
| SSRD-F-18, | 可靠性与恢复 | SensorSvc / App Logic | 功能用例 + 边界测试 |
| SSRD-F-15, | 安全与升级完整性 | SensorSvc / App Logic | 功能用例 + 边界测试 |

## 附录 B：STM32 ⇄ nRF 串口协议要点（摘要）

本附录给出协议最小可用定义，用于联调与测试。最终以 Stage4 SID 为准。

| 字段 | 字节 | 说明 |
| --- | --- | --- |
| SOF | 1 | 固定 0xA5 |
| VER | 1 | 协议版本 |
| TYPE | 1 | 消息类型 |
| LEN | 2 | payload 长度（小端） |
| PAYLOAD | N | 业务数据 |
| CRC16 | 2 | CRC16-CCITT |

建议消息：HR/SPO2/STEP/FALL/ALARM/GPS/TIME_SYNC/CFG_PUSH/LOG_PULL/OTA_CHUNK 等。

## 附录 C：关键图示

图 C‑1 连接/模块示意（工程资料）
