# os

[← 代码架构库](../../MOC.md) | [← 嵌入式工程架构](../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../MOC.md) | [← 主页](../../../../index.md)

---

存放 FreeRTOS 等操作系统源码，不放业务任务和 OSAL Wrapper。

| 内容 | 作用 |
| --- | --- |
| [OSAL公开接口](../04_platform/platform_os/inc/MOC.md) | App和Service使用的统一OS接口 |
| [OSAL Wrapper](../04_platform/platform_os/os_wrapper/MOC.md) | 参数检查、类型转换和框架逻辑 |
| [OSAL Implementation](../04_platform/platform_os/os_implementation/MOC.md) | 具体RTOS API适配 |
| [FreeRTOS移植与使用](../../../../术中自有万钟粟/嵌入式工程架构/freeRTOS/MOC.md) | FreeRTOS源码移植和配置 |

## os配置:

1. 从 [FreeRTOS移植笔记](../../../../术中自有万钟粟/嵌入式工程架构/freeRTOS/移植所有文件.md) 复制内核、Port、内存管理和配置文件。
2. 复制 `firmware/04_platform/platform_os/` 下的三个源码目录。
3. 工程只选择一个 FreeRTOS `heap_x.c`。
4. App 只调用 OSAL，不直接调用 FreeRTOS API。
