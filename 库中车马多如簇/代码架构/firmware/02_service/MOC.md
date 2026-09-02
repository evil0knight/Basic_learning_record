# service

[← 代码架构库](../../MOC.md) | [← 嵌入式工程架构](../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../MOC.md) | [← 主页](../../../../index.md)

---

按业务功能建立 service_xxx 文件夹，编排业务流程并对 App 提供接口。

| 内容 | 作用 |
| --- | --- |
| service_xxx.h / service_xxx.c | 声明并实现业务服务 |
| module_task/ | 服务内部的周期任务或功能模块 |
| platform/ | 服务内部使用的平台端口 |

## service_xxx配置:

1. 新建目录：`Service/service_xxx/`。
2. 新建 `service_xxx.h`，只声明 App 需要调用的接口：

```c
void service_xxx_init(void);
void service_xxx_process(void);
```

3. 在 `service_xxx.c` 实现接口，内部调用 Platform Wrapper 或 OSAL。
4. 需要独立任务时，新建 `module_task/xxx_task.h/.c`，再按 [app_init](../01_app/app_init/MOC.md) 加入任务表。
5. 需要平台差异时，在 `platform/xxx_port.h/.c` 封装，不在 Service 中包含 HAL。
