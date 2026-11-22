# ADC 工程代码

[← ADC 总览](../MOC.md) | [← FOC 工程代码](../../FOC/MOC.md#工程代码索引) | [← 主页](../../../index.md)

---

## 通用双缓冲版

面向 RTOS 或主循环消费，App 承接 HAL 回调，Service 用两块缓冲区和两个任务完成搬运与处理。

| 层 | 代码 | 作用 |
| --- | --- | --- |
| App | [user_isr_handlers.c](./通用双缓冲版/01_app/app_isr_handlers/user_isr_handlers.c) / [user_isr_handlers.h](./通用双缓冲版/01_app/app_isr_handlers/user_isr_handlers.h) | HAL ADC 回调入口，转发 Platform |
| Platform | [adc_port.c](./通用双缓冲版/04_platform/platform_mcu/adc/src/adc_port.c) / [adc_port.h](./通用双缓冲版/04_platform/platform_mcu/adc/inc/adc_port.h) | ADC 实例、HAL 封装、回调和事件分发 |
| 配置 | [adc_port_config.h](./通用双缓冲版/04_platform/platform_mcu/adc/inc/adc_port_config.h) | 实例、HAL 句柄和超时配置 |
| Service | [service_adc.c](./通用双缓冲版/02_service/service_adc/service_adc.c) / [service_adc.h](./通用双缓冲版/02_service/service_adc/service_adc.h) | 双缓冲状态、搬运任务和采样任务 |

详细说明见 [通用双缓冲版 MOC](./通用双缓冲版/MOC.md)。

## FOC简化版

面向 FOC 电流环，增加 Injected 组采样、内部校准和注入完成回调，服务层直接注册回调消费。

| 文件 | 作用 |
| --- | --- |
| [adc_port.c](./FOC简化版/04_platform/platform_mcu/adc/src/adc_port.c) | 规则组和 Injected 组启动停止、校准、回调注册和 IRQ 分发 |
| [adc_port.h](./FOC简化版/04_platform/platform_mcu/adc/inc/adc_port.h) | `core_adc_*` 接口、实例枚举和回调类型 |
| [adc_port_config.h](./FOC简化版/04_platform/platform_mcu/adc/inc/adc_port_config.h) | ADC1/2/3 句柄、Injected 通道数量和超时 |

调用链：

```text
HAL_ADCEx_InjectedConvCpltCallback
    -> core_adc_irq_injected_complete
    -> service_foc_motor1_foc_task
    -> core_adc_get_injected_value
    -> bsp_wrapper_current_read
```

## 两版差异

| 对比项 | 通用双缓冲版 | FOC简化版 |
| --- | --- | --- |
| 采样组 | 普通规则组、DMA、中断 | 普通规则组 + Injected 组 |
| 数据消费 | Service 双任务双缓冲 | FOC 中断直接读取并注册回调 |
| 校准 | 基础初始化 | 增加 `core_adc_calibrate()` |
| 实例 | ADC1 为主 | ADC1、ADC2、ADC3 |
| 适用 | RTOS、通用采样 | 电流环、温度采样、FOC 高实时 |
