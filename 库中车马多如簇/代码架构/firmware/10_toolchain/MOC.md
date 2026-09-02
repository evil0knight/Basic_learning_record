# toolchain

[← 代码架构库](../../MOC.md) | [← 嵌入式工程架构](../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../MOC.md) | [← 主页](../../../../index.md)

---

存放 Keil、CMake 等工程文件、链接脚本和构建配置。

| 文件/目录 | 作用 |
| --- | --- |
| `Toolchain/keil/` | Keil工程文件 |
| `Toolchain/iar/` | IAR工程文件 |
| `Toolchain/cmake/` | CMake构建文件 |
| `Toolchain/linker/` | 链接脚本 |

## toolchain配置:

1. 选择一种编译器工程：Keil、IAR 或 CMake。
2. 添加 `firmware/`、`Platform/`、`Bsp/`、`Core/` 和 `Drivers/` 源文件。
3. 配置所有 `inc/`、`Inc/` 目录为头文件搜索路径。
4. 配置芯片、编译器、链接脚本和启动文件。
5. 重新生成工程时检查新增文件是否仍在工程列表中。
