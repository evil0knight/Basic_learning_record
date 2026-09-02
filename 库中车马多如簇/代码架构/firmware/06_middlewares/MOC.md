# middlewares

[← 代码架构库](../../MOC.md) | [← 嵌入式工程架构](../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../MOC.md) | [← 主页](../../../../index.md)

---

存放 GUI、文件系统、通信协议、算法和其他第三方中间件，不放项目业务代码。

| 文件/目录 | 作用 |
| --- | --- |
| `middlewares/xxx/` | 第三方库源码 |
| `middlewares/xxx/xxx_config.h` | 第三方库配置 |

## middlewares配置:

1. 每个第三方库单独建立目录，例如 `middlewares/xxx/`。
2. 将库的头文件加入工程路径，将需要的 `.c` 加入工程。
3. 库的配置文件放在该库目录，不放入 App。
4. 由 Service 或 Platform Wrapper 封装第三方库接口，业务层不直接依赖第三方类型。
