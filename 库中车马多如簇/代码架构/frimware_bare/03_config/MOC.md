# 03_config

[返回工程根目录](../MOC.md) | [返回代码架构](../../MOC.md) | [返回主页](../../../../index.md)

## 关键文件

- `board_config.h`：板级型号和硬件资源配置。
- `feature_config.h`：功能开关配置。
- `project_config.h`：工程配置统一入口和裸机运行参数。
- `cubemx/project.ioc` / `cubemx/.mxproject`：CubeMX 芯片、外设和工程生成配置。
- `version_config.h`：固件版本信息。

## 调用顺序

```text
project_config.h
    -> board_config.h / feature_config.h
    -> cubemx/project.ioc
    -> platform / BSP / application
```


