# drivers

[← 代码架构库](../../MOC.md) | [← 嵌入式工程架构](../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../MOC.md) | [← 主页](../../../../index.md)

---

存放 ARM CMSIS、芯片启动文件和芯片厂商 HAL 驱动。

| 文件/目录 | 作用 |
| --- | --- |
| `Drivers/CMSIS/` | ARM内核、设备和编译器接口 |
| `Drivers/STM32xx_HAL_Driver/` | 芯片厂商HAL驱动 |
| `startup_xxx.s` | 芯片启动文件 |

## drivers配置:

1. 将芯片软件包中的 `CMSIS`、启动文件和 HAL 复制到 `Drivers/`。
2. 将芯片头文件、HAL 头文件加入工程头文件路径。
3. 只在 `platform_mcu/*/src/` 调用 HAL。
4. App、Service、BSP Wrapper 不直接包含 HAL 头文件。
