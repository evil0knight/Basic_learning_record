# bsp_adapter

[← 代码架构库](../../../../MOC.md) | [← 嵌入式工程架构](../../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../../MOC.md) | [← 主页](../../../../../../index.md)

---

连接 Wrapper、BSP、MCU 和 OS，负责实例化器件并向 Wrapper 注册函数表。

| 文件 | 作用 |
| --- | --- |
| [bsp_adapter_port_xxx.h](./bsp_adapter_port_xxx.h) | 可复用：Adapter注册入口声明 |
| [bsp_adapter_port_xxx.c](./bsp_adapter_port_xxx.c) | 复用挂载结构，替换MCU/OS Port实现 |

## Adapter层职责

1. Adapter 连接具体工程与通用 BSP，负责组装 MCU、OS、Driver、Handler 和 Wrapper。
2. 对外只提供 `bsp_adapter_xxx_register()`，在 `app_periph_register()` 中调用。
3. 注册时将 Adapter 的初始化、控制桥接函数交给 Wrapper 保存。
4. 初始化桥接函数依次绑定 Driver Port、初始化 Driver、初始化 Handler、注册 Driver。
5. 多实例按固定顺序注册，Handler 返回的 `index` 对应 `driver_group[index]`。

## bsp_adapter配置:

1. 新建 `bsp_adapter_port_xxx.h/.c`。
2. 头文件只声明一个注册入口：

```c
void bsp_adapter_xxx_register(void);
```

3. 复制现有骨架，修改 `bsp_adapter_xxx_port_init()` 和 `bsp_adapter_xxx_port_control()`。
4. 在 `app_init/platform_io_register/user_periph_setup.c` 调用注册入口。
5. Adapter 负责挂载，业务代码不直接调用 Adapter 内部函数。

SPI器件的Driver保存静态表槽位编号。Adapter不保存物理引脚；实际`hspi`和CS填写在`spi_port_config.h`静态表，参见[SPI Port](../../../../../SPI/SPI_Port/MOC.md)。
