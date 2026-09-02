# ui

[← 代码架构库](../../../MOC.md) | [← 嵌入式工程架构](../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../MOC.md) | [← 主页](../../../../../index.md)

---

存放页面、控件、显示刷新和输入事件，只调用 Service 或 Platform Wrapper 提供的接口。

| 内容 | 作用 |
| --- | --- |
| 文件/目录 | 作用 |
| --- | --- |
| `ui_init.h / ui_init.c` | UI统一初始化入口 |
| `pages/` | 页面及页面切换逻辑 |
| `widgets/` | 可复用控件 |
| `ui_event.h / ui_event.c` | 输入事件接收与分发 |

## ui配置:

1. 新建 `ui/ui_init.h/.c`，提供 UI 初始化入口。
2. 页面放入 `ui/pages/`，控件放入 `ui/widgets/`。
3. 输入事件统一放入 `ui/ui_event.h/.c`。
4. UI 只调用 Service 或 Wrapper，不直接调用 HAL。
5. UI 刷新需要独立任务时，按 [app_init](../app_init/MOC.md) 加入任务表。
