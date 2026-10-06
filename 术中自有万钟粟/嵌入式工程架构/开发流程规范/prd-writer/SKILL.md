---
name: prd-writer
description: 撰写产品需求文档(PRD)的交互式写作向导。当用户要起草、编写、制定产品需求文档、PRD、产品需求规格说明书、需求描述时使用。澄清前联网调研目标用户与行业最新解决方案,供用户选择方案;需求澄清阶段严格遵循内嵌的 Superpowers brainstorming 方法论(原封不动);以提炼的 PRD 教程方法论为书写推理依据;再将澄清成果落地为 11 节标准 PRD。
---

# PRD 产品需求文档写作向导

本 skill 把 Superpowers 的 **brainstorming** skill(原封不动,见 `references/brainstorming.md`)作为需求澄清的核心流程,以 `references/prd-methodology.md`(提炼自 PRD 教程原文)作为书写推理依据,澄清获批后落地为 11 节标准 PRD 结构(见 `references/prd-template.md`)。

> 前置依赖:PRD 必须在 BRD 确定后编写。没有 BRD 作为需求支撑,不产出正式 PRD;若尚无 BRD,先引导用户完成 BRD(见 `brd-writer`)。

## 执行流程

### 第一步:需求澄清 —— 严格遵循 brainstorming(原封不动)

完整、不省略地遵循 `references/brainstorming.md` 的整套方法论:
- 先做路径分类(Spike / Bounded / Architectural),并**说给用户听**以便纠正
- 遵守 HARD-GATE:未经用户确认设计意图,不产出任何文档
- 一次只问一个问题,优先选择题
- 走完"用户审批设计"这道门,才进入下一步

brainstorming 在 PRD 场景下的路径映射(唯一本地桥接):

| brainstorming 路径 | 对应 PRD 场景 |
|---|---|
| Spike | 可行性问题 —— 只要一个判断,不产出正式 PRD |
| Bounded | 更新现有 PRD —— 只澄清受影响的点,产出最小改动 |
| Architectural | 全新 PRD —— 完整澄清 + 完整 11 节 PRD |

### 第二步:行业调研(目标用户 + 解决方案)与方案选择

承接 brainstorming 的「Exploring approaches / Propose 2-3 approaches」阶段。动手前先联网调研,分两块:
1. **目标用户/干系人**:用 WebSearch 检索「现在都有谁想要/使用这个产品」,落到具体角色(不同产品用户不同,不套固定模板),写入第 3 节。
2. **行业最新解决方案**:检索当前行业最新、主流的技术与产品方案(关键词覆盖技术路线、竞品、开源方案、标准),提炼 2-3 个候选方案,各给出权衡(trade-offs)与推荐理由。
3. **交用户选择方案**——一次一个选择题,推荐项放首位;用户选定前不写 PRD。

> 方案未获用户选定,视为 HARD-GATE 未过,不得进入第三步。

### 第三步:桥接到 PRD 结构

brainstorming 原文的结尾是"调用 writing-plans / 写代码";本 skill 把它改为:把获批的设计意图(含用户选定的方案)映射到 11 节标准 PRD(见 `references/prd-template.md`),并严格依据 `references/prd-methodology.md` 的推理依据书写:
- 一句话定位与产品定义 → 第 2 节(统一下所有人对产品是什么、解决什么问题的看法)
- 干系人、场景与关键任务 → 第 3~4 节
- 目标、成功指标与范围 → 第 5~6 节
- 产品能力与性能约束 → 第 7~8 节
- 验收场景与 Owner/追溯 → 第 9~10 节
- 按 ID 规范分配编号
- 未确认信息标 `待补充`,不编造

### 第四步:生成草稿

按 `references/prd-template.md` 生成完整 11 节 PRD,输出到当前目录 `PRD-<项目名>.md`。

### 第五步:自检 + 评审 + 迭代

自检:11 节齐全、ID 唯一、追溯闭合、量化口径前后一致、无串味、无占位/矛盾/歧义/越界。然后请用户评审,按反馈迭代。

## 核心原则

1. 审批门(HARD-GATE):未确认不落笔
2. 先调研后提案:目标用户与方案均须参考联网调研,由用户选定;用户 Person 按产品定人,不套固定模板
3. 咬文嚼字式拆解:把模糊需求拆成可量化、可验证的子项(如"精度"→ 测量对象 + 精度 + 测量时间 + 测量范围),每个指标都必须有「通过口径」和「用户可见证据」;用户不关心内部指标,只关心可对比、可复现的结果
4. 不编造事实:未验证的标 `待补充`
5. 不静默留空:标 `待补充`
6. 可追溯:BRD → PRD → SyRS,每个需求落到具体 Owner

## ID 规范

| 前缀 | 用途 | 章节 |
|------|------|------|
| `PRD-GOAL-xxx` | 产品目标 | 5 |
| `PRD-SC-xxx` | 场景/用户任务需求 | 4 |
| `PRD-FR-xxx` | 功能需求 | 7、10 |
| `PRD-NFR-xxx` | 非功能需求 | 8、10 |
| `PRD-ACC-xxx` | 验收需求 | 9、10 |

编号从 001 递增,同一前缀内不重复。

## 治理与变更规范

任何 P0 范围、验收阈值或 Owner 变更,必须记录:影响、证据和批准人。变更 → 记录 → 对应权威源 Owner 更新版本 → 审计后生效。

## 追溯规范

- PRD 到 SyRS 的追溯只指向 SYS ID,不越过 SyRS 直接指向实现任务
- 每条 P0 PRD 需求至少有一条 P0 BRD 入向追溯
- 正式 Trace 字段只引用现有 SYS ID

## 参考

- `references/brainstorming.md` —— 内嵌的 Superpowers brainstorming 方法论(原封不动;MIT,作者 Jesse Vincent / obra)
- `references/visual-companion.md` —— brainstorming 的可视化配套(brainstorming.md 内原引用路径为 `skills/brainstorming/visual-companion.md`)
- `references/prd-methodology.md` —— 书写推理依据(提炼自 PRD 教程原文)
- `references/prd-template.md` —— 11 节标准 PRD 模板
