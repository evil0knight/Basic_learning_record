# IIC

[← 模块总览](../MOC.md) | [← 主页](../../index.md)

> [IIC知识](./知识/MOC.md)

---

| 内容                           | 文件                              |
| ------------------------------ | --------------------------------- |
| 硬件I2C、软件I2C与统一总线接口 | [I2C Port](./IIC_Port/MOC.md)        |
| HAL库硬件I2C                   | 由CubeMX生成 `i2c.h`、`i2c.c` |

## 移植

1. 建 `firmware/04_platform/platform_mcu/i2c/`，复制[I2C Port](./IIC_Port/MOC.md)源码。
2. 硬件I2C由CubeMX配置并生成 `i2c.h`、`i2c.c`；软件I2C将SCL、SDA配置为开漏输出。
3. 给引脚设置User Label，确认物理资源宏已生成到 `main.h`。
4. 在 `i2c_port_config.h`静态表中为每条物理总线分配一个槽位。
5. Driver保存槽位编号和7位设备地址，只调用 `core_i2c_*()`。
