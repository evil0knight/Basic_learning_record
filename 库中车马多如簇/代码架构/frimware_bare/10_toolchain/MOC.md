# 10_toolchain

[返回工程根目录](../MOC.md) | [返回代码架构](../../MOC.md) | [返回主页](../../../../index.md)

## 关键文件

- `MDK-ARM/project.uvprojx`：Keil 工程文件和源文件列表。
- `MDK-ARM/project.uvoptx`：Keil 调试和下载配置。
- `MDK-ARM/DebugConfig/`：调试器配置。
- `MDK-ARM/RTE/`：Keil 运行时环境配置。
- `MDK-ARM/stm32g474xx_flash.sct`：Flash/RAM 链接布局。
- `.vscode/`、构建脚本和其他编译配置：工程构建辅助文件。

## 调用顺序

```text
MDK-ARM / build configuration
    -> compiler and assembler
    -> linker script
    -> firmware image
```
