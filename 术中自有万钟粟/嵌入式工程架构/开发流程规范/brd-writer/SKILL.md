---
name: brd-writer
description: 撰写商业需求文档(BRD)的交互式写作向导。当用户要起草、编写、制定商业需求文档、BRD、需求规格说明书时使用。需求澄清阶段严格遵循内嵌的 Superpowers brainstorming 方法论(原封不动),再将澄清成果落地为 15 节标准 BRD。
---

# BRD 商业需求文档写作向导

本 skill 把 Superpowers 的 **brainstorming** skill(原封不动,见 `references/brainstorming.md`)作为需求澄清的核心流程,澄清获批后落地为 15 节 BRD 结构(见 `references/brd-template.md`),并以 `references/brd-reasoning.md`(BRD 会议记录文字)作为章节写作推理依据。方案探索阶段须先联网搜索行业最新解决方案,由用户选定方案后再落笔。

## 执行流程

### 第一步:需求澄清 —— 严格遵循 brainstorming(原封不动)

完整、不省略地遵循 `references/brainstorming.md` 的整套方法论:
- 先做路径分类(Spike / Bounded / Architectural),并**说给用户听**以便纠正
- 遵守 HARD-GATE:未经用户确认设计意图,不产出任何文档
- 一次只问一个问题,优先选择题
- 走完"用户审批设计"这道门,才进入下一步

brainstorming 在 BRD 场景下的路径映射(唯一本地桥接):

| brainstorming 路径 | 对应 BRD 场景 |
|---|---|
| Spike | 可行性问题 —— 只要一个判断,不产出正式 BRD |
| Bounded | 更新现有 BRD —— 只澄清受影响的点,产出最小改动 |
| Architectural | 全新 BRD —— 完整澄清 + 完整 15 节 BRD |

方案探索阶段(brainstorming 的 "Exploring approaches")必须遵守下方『行业方案搜索规则』:先联网搜索行业最新解决方案,整理成选项交用户选定,再继续。

### 第二步:桥接到 BRD 结构

brainstorming 原文的结尾是"调用 writing-plans / 写代码";本 skill 把它改为:把获批的设计意图映射到 15 节 BRD(见 `references/brd-template.md`):
- 设计意图 → 第 2~7 节(定位 / 背景 / 干系人 / 目标 / 价值 / 范围)
- 按 ID 规范分配编号
- 未确认信息标 `待补充`,不编造

### 第三步:生成草稿

写作推理依据:`references/brd-reasoning.md`(BRD 书写会议记录文字)—— 逐节对照该记录,理解"这一节为什么写、如何从需求/会议结论推导",再按 `references/brd-template.md` 生成完整 15 节 BRD,输出到当前目录 `BRD-<项目名>.md`。

### 第四步:自检 + 评审 + 迭代

自检:15 节齐全、ID 唯一、追溯闭合、无串味、无占位/矛盾/歧义/越界。然后请用户评审,按反馈迭代。

## 核心原则

1. 审批门(HARD-GATE):未确认不落笔
2. 不编造事实:未验证的进第 14 章
3. 不静默留空:标 `待补充`
4. 可追溯
5. 行业方案先联网搜索,由用户选定后落笔

## 行业方案搜索规则

在方案探索阶段,不得仅凭内部知识直接给出方案,必须先联网搜索行业最新解决方案:

1. **联网搜索**:用 WebSearch 检索与本 BRD 主题相关的行业最新解决方案(竞品方案、开源方案、标准做法、新器件/新技术/新架构)。
2. **参考引用**:把搜索结果作为方案依据,并注明来源(厂商/方案名/链接)。
3. **呈现选择**:整理 2-3 个行业方案为选项,附各自权衡与你的推荐,交由用户选择。
4. **以用户选定为准**:用户选定后,才进入 BRD 结构映射与草稿生成;未选定不落笔。

未联网搜索、未经用户选定方案,不得推进到下一步。

## ID 规范

| 前缀 | 用途 | 章节 |
|------|------|------|
| `BRD-STK-xxx` | 干系人 | 4 |
| `BRD-OBJ-xxx` | 业务需求 | 8 |
| `BRD-NFR-xxx` | 非功能需求 | 8 |
| `BRD-ASM-xxx` | 约束/假设/依赖 | 9 |
| `BRD-RSK-xxx` | 风险 | 11 |
| `BRD-ASM-1xx` | 待验证假设 | 14 |

编号从 001 递增,同一前缀内不重复。

## 治理与变更规范

任何 P0 范围、需求、成功门槛、Owner 或目标日期变更,必须记录:原因、影响、受影响的 PRD/SyRS 与证据、回退方案、批准人。变更 → 记录 → 对应权威源 Owner 更新版本 → 审计后生效。

## 追溯规范

- BRD 到 PRD 的追溯只指向 PRD ID,不绕过 PRD 直接指向 SYS 或 IC
- 每条 P0 PRD 需求至少有一条 P0 BRD 入向追溯
- 正式 Trace 字段只引用现有 PRD ID

## 参考

- `references/brainstorming.md` —— 内嵌的 Superpowers brainstorming 方法论(原封不动;MIT,作者 Jesse Vincent / obra)
- `references/visual-companion.md` —— brainstorming 的可视化配套(brainstorming.md 内原引用路径为 `skills/brainstorming/visual-companion.md`)
- `references/brd-template.md` —— 15 节 BRD 模板
- `references/brd-reasoning.md` —— BRD 书写会议记录文字(写作推理依据,源自 `BRD会议记录文字.md`)
