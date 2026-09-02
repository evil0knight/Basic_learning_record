# bsp_wrapper

[← 代码架构库](../../../../MOC.md) | [← 嵌入式工程架构](../../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../../MOC.md) | [← 主页](../../../../../../index.md)

---

向 App、Service 提供稳定接口，隐藏 BSP 类型、具体器件和平台细节。

| 文件 | 作用 |
| --- | --- |
| [bsp_wrapper_xxx.h](./bsp_wrapper_xxx.h) | 可复用：App调用的稳定接口 |
| [bsp_wrapper_xxx.c](./bsp_wrapper_xxx.c) | 可复用：保存Adapter函数表并转调 |

## Wrapper层职责

1. Wrapper 是 App、Service 使用的稳定接口，不暴露 Driver、Handler、MCU 和 OS 类型。
2. Wrapper 保存 Adapter 注册的函数表，在 `init`、`control` 等公开接口中转调。
3. 多实例由 App 传入逻辑 `index`，Adapter 将其交给 Handler 查找 `driver_group[index]`。
4. 调用顺序：`app_periph_register()` 注册 Adapter，系统初始化调用 Wrapper `init`，业务代码只调用 Wrapper API。

## bsp_wrapper配置:

1. 复制 `bsp_wrapper_xxx.h/.c`，将 `xxx` 改为模块名。
2. 在头文件中定义 App 使用的类型和接口，不暴露 Driver 类型：

```c
void bsp_wrapper_xxx_init(void);
int32_t bsp_wrapper_xxx_control(uint32_t index, uint32_t command,
                                const void *argument);
```

3. 在源文件中保存 Adapter 注册的函数表。
4. App 只调用 Wrapper 的接口。

Wrapper不包含`main.h`、HAL或SPI Port类型。外部Flash、屏幕使用硬件SPI还是软件SPI，不改变Wrapper公开接口。
