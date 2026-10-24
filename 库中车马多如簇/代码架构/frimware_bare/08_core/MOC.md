# 08_core

[返回工程根目录](../MOC.md) | [返回代码架构](../../MOC.md) | [返回主页](../../../../index.md)

## 关键文件

- `Core/Inc/`：CubeMX 生成的系统、外设和中断头文件。
- `Core/Src/main.c`：工程启动入口。
- `Core/Src/stm32g4xx_it.c`：中断入口和 HAL 中断转发。
- `Core/Src/system_stm32g4xx.c`：系统时钟和芯片启动支持。
- `Core/Src/*_msp.c`：HAL 外设底层初始化。
- `startup_stm32g474xx.s`：芯片启动汇编文件。

## 调用顺序

```text
startup_stm32g474xx.s
    -> system_stm32g4xx.c
    -> main.c
    -> HAL / Platform 初始化
    -> 裸机主循环
```

