# 02_service

[返回 frimware_bare](../MOC.md) | [返回代码架构](../../MOC.md) | [返回主页](../../../../index.md)

## 关键文件

Service 按具体工程功能建立。状态转移表中的 `pf_action` 绑定实际 Service 函数，Service 通过既有 Wrapper 接口访问设备。

## 调用顺序

```text
app_control_event_map(EVENT_xxx)
    -> transition table.pf_action
    -> 工程 Service
    -> Wrapper
    -> Adapter
    -> Handler
    -> Driver
```

