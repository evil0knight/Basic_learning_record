# bsp_driver

[← 代码架构库](../../../../MOC.md) | [← 嵌入式工程架构](../../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../../MOC.md) | [← 主页](../../../../../../index.md)

---

实现寄存器读写和单个器件的实例化、初始化及基础控制。

| 文件 | 作用 |
| --- | --- |
| [bsp_xxx_driver.h](./bsp_xxx_driver.h) | 可复用：Driver对象和Port接口 |
| [bsp_xxx_driver.c](./bsp_xxx_driver.c) | 可复用：参数检查、初始化和控制转调 |

## Driver层职责

1. 定义 Driver 对象，保存器件参数、Port 函数表和 Port 上下文。
2. 使用 `bsp_xxx_driver_bind()` 将 Port 函数表绑定到 Driver，实现硬件无关。
3. `bsp_xxx_driver_control()` 检查对象状态后，通过函数指针调用底层操作。
4. 所有公开函数先检查参数和初始化状态；调试输出通过配置宏按需增加。

## bsp_driver配置:

1. 复制 `bsp_xxx_driver.h/.c`，将 `xxx` 改为模块名。
2. 头文件声明单个器件的初始化和基础控制接口：

```c
typedef struct
{
    void *port;
} bsp_xxx_driver_t;

int32_t bsp_xxx_driver_init(bsp_xxx_driver_t *driver);
int32_t bsp_xxx_driver_control(bsp_xxx_driver_t *driver,
                               uint32_t command, void *argument);
```

3. 源文件只实现器件基础控制，不管理任务、队列和业务状态。
