# ST7789

[← 屏幕模块](../MOC.md) | [← 主页](../../../index.md)

---

| 文件 | 作用 |
| --- | --- |
| [st7789.h](./st7789.h) | 显示实例和绘图接口 |
| [st7789.c](./st7789.c) | 初始化、窗口、填色、图像、休眠和唤醒 |

## 接线与配置

1. 在CubeMX配置SPI、CS、DC和RST，给引脚设置User Label。
2. 按[SPI Port](../../SPI/SPI_Port/MOC.md)填写静态表，并记录屏幕对应的槽位。
3. 将DC、RST注册到`CORE_GPIO_DISPLAY_DC`、`CORE_GPIO_DISPLAY_RESET`。
4. 完成静态表和GPIO配置后初始化屏幕：

```c
static st7789_t display = {
    .width = 240U,
    .height = 240U,
    .spi_port = project_display_spi_port,
    .delay_ms = HAL_Delay
};

st7789_init(&display);
st7789_fill(&display, 0xF800U);
```

Driver使用实例中的`spi_port`，不保存`hspi`和物理引脚。换板或切换软件SPI时只修改`spi_port_config.h`静态表，屏幕源码不变。

当前实现为阻塞传输。大面积刷新需要进一步在SPI Port增加DMA接口，并在DMA完成回调后释放事务。
