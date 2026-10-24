# 06_middlewares

[返回工程根目录](../MOC.md) | [返回代码架构](../../MOC.md) | [返回主页](../../../../index.md)

## 关键文件

- `algorithm/`：PID、滤波、传感器和其他通用算法。
- `communication/`：YMODEM、日志、调试和通信协议组件。
- `file_system/`：LittleFS 等文件系统组件及其 Port。
- `LVGL/`：图形界面中间件及其配置。
- `vendor/`：芯片厂商和工具链带入的中间件，例如 ST、Third_Party 组件。

## 调用顺序

```text
algorithm / communication / file_system / LVGL
    -> Service 或 Platform Wrapper
    -> application

vendor
    -> 中间件适配层
    -> Service / Platform Wrapper
```
