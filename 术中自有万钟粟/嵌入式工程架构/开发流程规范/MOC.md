# 开发流程规范 MOC

[← 返回嵌入式工程架构](../MOC.md) | [← 返回技术栈](../../MOC.md) | [← 主页](../../../index.md)

> [参考项目](参考项目介绍.md)

> [项目目录模板](./项目目录模板.md)：软件工程师的工程目录与 GitHub 模板。本页汇总全周期开发流程和阶段文档。

---

## 黑话:

* **冻结**:产品进入只修缺陷、不加新功能的阶段。需求与代码基线锁定,任何改动都要走变更评审。
* **敏捷开发**:把大目标切成 2~4 周的小迭代,每个迭代结束都要有一个能演示的可用产物;需求允许边做边改。
  **瀑布开发**:需求 → 设计 → 实现 → 测试 → 量产 严格按顺序走,上一阶段不通过不进入下一阶段。需求稳定时省心,需求一变就全盘返工。
* **wiki文档**:团队共用的在线文档库,用来沉淀规范、接口约定和决策记录。和代码一样需要有人维护,否则会烂尾。
* **基线**:某个时间点锁定、之后受控的版本。后续所有改动都以它为起点比对,不能随便动。
* **门禁**:两个阶段之间的质量关卡。规定的交付物不齐、指标不达标,就不放行到下一阶段。
* **里程碑**:计划里的关键时间点,通常对应一次门禁或一次交付。
* **追溯**:需求 → 设计 → 代码 → 测试 每一环都能双向倒查,证明"每条需求都被实现、且被验证过";覆盖率就是能倒查到的需求占比。
* **Sprint**:敏捷里的固定开发周期,一般 2~4 周,周期结束要交出一个能演示的产物。
* **MVP**:最小可行产品。只做能验证核心假设的那部分功能,先证明方向对,再补全。
* **REL / TR 编号**:`REL-x.x.x` 是发布基线号,`TR-x.x.x` 是对应的测试报告号,靠编号把某个版本和它的验证证据绑在一起。
* **P0 / P1 / P2**:优先级分级。P0 最高(不解决就不能发布),依次递减。
* **冒烟测试**:先跑一遍最主干的功能,确认整个系统没塌,再投入正式测试。
* **复盘**:一轮做完回头看过程,找可复用的经验和该改的流程,对事不对人。
* **DFM**:面向制造的设计。在画图阶段就考虑产线做不做得出来、好不好装,别等量产才发现装不上。
* **DFMEA**:设计失效模式与影响分析。提前列出每个部件可能怎么坏、坏了什么后果、靠什么措施防住。

## 文档清单

1. [用户需求文档：User Needs Document（UND）](./UND.md)
2. [干系人需求文档：Stakeholder Needs Document（SND）](./SND.md)
3. [需求价值分析文档：Kano Analysis Document（KAD）](./KAD.md)
4. [需求映射与权重分析：Quality Function Deployment（QFD）](./QFD.md)
5. [业务需求文档：Business Requirements Document（BRD）](./BRD.md),
   这个文件夹里有个 `brd-writer`的文件夹里面是一个skill(立芯嵌入式拷贝来的)
6. [产品需求文档：Product Requirements Document（PRD）](./PRD.md)
   这个文件夹里有个 `prd-writer`的文件夹里面是一个skill(立芯嵌入式拷贝来的)
7. [系统需求文档：System Requirements Specification（SyRS / SRSys）](./SyRS.md)
8. [系统架构文档：System Architecture Document（SAD）](./SAD.md)
9. [需求分配矩阵：Requirement Allocation Matrix（RAM）](./RAM.md)
10. [架构决策记录：Architecture Decision Record（ADR）](./ADR.md)
11. [系统需求规格说明书 Software System Requirements Definition（SSRD）](./SSRD.md)
12. [系统需求规格说明书 Software Requirements Specification（SRS）](./SRS.md)
13. [硬件能力手册 Hardware Requirements Specification（HRS）](./HRS.md)
14. [软件设计说明书 Software Design Description（SDD）](./SDD.md)
15. [硬件设计文档 Hardware Design Document (HDD)](./HDD.md)
16. [敏捷规划与需求-代码-测试追踪方案（Agile Plan）](./Agile-Plan.md)
