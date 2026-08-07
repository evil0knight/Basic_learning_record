# 单片机烧录：从 RAM 到 Flash

[← 返回 编译链接下载](./MOC.md) | [← 主页](../../index.md)

> [修改jlink算法下载到外部flash](https://twd6onxsxva.feishu.cn/docx/VmjDdzhspo6NrXxds1ucIsu2nId)(未实验)
>
> [下载LVGL到外部的flash](https://twd6onxsxva.feishu.cn/docx/CWOXdid60o6R9Excg96cNGnnnOb)(未试验过)

---

**核心：仿真器并不直接把代码写入 Flash，而是先下载到 RAM，再让 MCU 自己分批搬运到 ROM。**

## 为什么不能直接下载到 ROM

1. **Flash 写入必须先擦除**，擦写时序复杂（解锁 → 写使能 → 查状态位），仿真器无法通晓所有 MCU 的 Flash 控制器细节
2. **总线冲突**：Flash 擦写时处于忙碌状态，若 CPU 同时在 Flash 取指令，直接 HardFault。烧录代码必须在 RAM 中独立运行

## 真实流程

```
仿真器 → RAM（下载算法 + 数据块） → MCU 自己烧进 Flash → 循环接力
```

**第一步 — 部署工具**：仿真器将编译好的"下载算法"（Keil 里 `.FLM` 文件，几 KB 的微型程序）写入 MCU 的 RAM 起始段。

**第二步 — 装载数据**：RAM 通常装不下完整固件，仿真器将业务程序切块，每次往 RAM 空闲区（Buffer）塞一小块（如 4KB）。

**第三步 — 内核干活**：仿真器修改 PC 指针，CPU 跳去 RAM 执行下载算法。该算法驱动 Flash 控制器，按正确时序将 Buffer 里的数据烧入 ROM。

**第四步 — 循环接力**：烧完一块，CPU 停机反馈，仿真器覆盖写入下一块到 RAM，指挥 CPU 继续烧，直到全部搬运完毕。
