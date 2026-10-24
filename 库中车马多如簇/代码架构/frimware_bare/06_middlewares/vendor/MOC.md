# vendor

[返回 06_middlewares](../MOC.md) | [返回工程根目录](../../MOC.md) | [返回主页](../../../../../index.md)

## 关键文件

- `ST/`：ST 提供的中间件组件。
- `Third_Party/`：随芯片包、工具链或外部工程引入的第三方组件。
- `xxx_config.h`：Vendor 中间件的独立配置文件。

## 调用顺序

```text
Vendor middleware
    -> vendor adapter / port
    -> Service 或 Platform Wrapper
```
