# config

[← 代码架构库](../../MOC.md) | [← 嵌入式工程架构](../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../MOC.md) | [← 主页](../../../../index.md)

---

存放全工程共用配置，不放业务实现。

| 文件 | 作用 |
| --- | --- |
| [board_config.h](./board_config.h) | 板型、引脚和板级差异配置 |
| [feature_config.h](./feature_config.h) | 功能开关和可选模块配置 |
| [project_config.h](./project_config.h) | 项目级公共参数 |
| [rtos_config.h](./rtos_config.h) | 任务栈、队列深度和 OS 资源参数 |
| [version_config.h](./version_config.h) | 固件版本信息 |

## config配置:

1. 从 `firmware/03_config/` 复制五个配置头文件。
2. 在 `board_config.h` 填写板型和引脚差异。
3. 在 `feature_config.h` 开关功能模块。
4. 在 `project_config.h` 汇总项目配置，业务代码只包含这个入口。
5. 在 `rtos_config.h` 填写任务栈、优先级、队列深度和信号量参数。
6. 在 `version_config.h` 修改固件版本号。
