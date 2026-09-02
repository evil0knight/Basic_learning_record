# core

[← 代码架构库](../../MOC.md) | [← 嵌入式工程架构](../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../MOC.md) | [← 主页](../../../../index.md)

---

存放 CubeMX 生成的 main.c、中断文件、系统时钟和片内外设初始化文件。

| 文件/目录 | 作用 |
| --- | --- |
| `Core/Inc/` | CubeMX生成的头文件 |
| `Core/Src/main.c` | 工程启动入口 |
| `Core/Src/stm32xx_it.c` | 中断入口 |
| `Core/Src/system_stm32xx.c` | 系统时钟和启动支持 |

## core配置:

1. 使用 CubeMX 或芯片工具生成 `Core/Inc` 和 `Core/Src`。
2. 保留 `main.c`、中断文件、系统时钟和 HAL 初始化代码。
3. 在 `main()` 完成 MCU 初始化后调用 [user_app_init](../01_app/app_init/MOC.md)。
4. 生成工具再次生成代码时，只修改用户代码区域。
