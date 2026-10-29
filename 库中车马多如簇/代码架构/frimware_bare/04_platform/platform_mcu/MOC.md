# platform_mcu

[←MOC](../MOC.md) | [←主页](../../../../../index.md)

---

## 外设索引

- [critical_port.h](./critical/inc/critical_port.h) /[critical_port.c](./critical/src/critical_port.c)：保存和恢复中断状态，
  为事件队列提供短临界区保护。接入时将实现文件加入编译，`main.h` 由目标工程提供；
  仅适用于单核 Cortex-M，NMI 和 HardFault 不得访问该队列。
- [gpio/MOC.md](./gpio/MOC.md)：GPIO 逻辑接口、板级静态配置表和 HAL 实现。
- [IIC/MOC.md](../../../../IIC/MOC.md)：I²C 的统一接口与 Port 实现。
- [SPI/MOC.md](../../../../SPI/MOC.md)：SPI 的统一接口与 Port 实现。
- [ADC/MOC.md](../../../../ADC/MOC.md)：ADC 统一接口与平台实现

## 目录规则

每个 MCU 外设单独建立目录，并使用：

```text
外设/
├── inc/    对上公开的 port 头文件
└── src/    具体 MCU/HAL 实现
```

上层只包含 `inc` 中的接口，不直接包含芯片 HAL 或访问 HAL 句柄。硬件初始化、NVIC、
DMA 和中断入口由具体平台工程实现；应用中断回调的职责见
[app_isr_handlers](../../01_app/MOC.md)。
