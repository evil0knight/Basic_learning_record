# 编码器工程代码

[← 编码器总览](../MOC.md) | [← FOC 工程代码](../../FOC/MOC.md#工程代码索引) | [← 主页](../../../index.md)

---

## 通用分层版

适合主循环轮询、RTOS 任务或控制频率不高的场景，保留完整的 BSP 分层。

| 层      | 代码                                                                                                                                                                                                      | 作用                                   |
| ------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------- |
| Service | [service_foc.c](./通用分层版/02_service/service_foc/service_foc.c) / [service_foc.h](./通用分层版/02_service/service_foc/service_foc.h)                                                                     | 初始化编码器、按节拍启动 DMA、缓存角度 |
| Adapter | [bsp_adapter_encoder.c](./通用分层版/04_platform/platform_bsp/encoder/adapter/bsp_adapter_encoder.c) / [bsp_adapter_encoder.h](./通用分层版/04_platform/platform_bsp/encoder/adapter/bsp_adapter_encoder.h) | 把 I2C 读写、异步提交和延时注入 Driver |
| Wrapper | [bsp_wrapper_encoder.c](./通用分层版/04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.c) / [bsp_wrapper_encoder.h](./通用分层版/04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h) | 设备注册、统一状态码和对外接口         |
| Handler | [bsp_encoder_handler.c](./通用分层版/05_bsp/bsp_encoder/handler/bsp_encoder_handler.c) / [bsp_encoder_handler.h](./通用分层版/05_bsp/bsp_encoder/handler/bsp_encoder_handler.h)                             | AS5600 寄存器配置、范围、滤波和 OTP    |
| Driver  | [bsp_encoder_driver.c](./通用分层版/05_bsp/bsp_encoder/driver/bsp_encoder_driver.c) / [bsp_encoder_driver.h](./通用分层版/05_bsp/bsp_encoder/driver/bsp_encoder_driver.h)                                   | 原始寄存器读写原语                     |

详细资源挂载、接口和标定说明见 [通用分层版 MOC](./通用分层版/MOC.md)。

## FOC实时版

面向 FOC 电流环中断，删除 Adapter、Handler、Driver 层，Wrapper 直接依赖简化平台接口。

| 文件                                                                                                       | 作用                                                 |
| ---------------------------------------------------------------------------------------------------------- | ---------------------------------------------------- |
| [bsp_wrapper_encoder.c](./FOC实时版/04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.c)         | 启动/停止 DMA、消费角度、外推角度与测速、配置 AS5600 |
| [bsp_wrapper_encoder.h](./FOC实时版/04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder.h)         | Wrapper 状态码与高实时接口                           |
| [bsp_wrapper_encoder_reg.h](./FOC实时版/04_platform/platform_bsp/encoder/wrapper/bsp_wrapper_encoder_reg.h) | AS5600 地址及寄存器定义                              |

调用链：

```text
FOC 服务/控制中断
    -> bsp_wrapper_encoder_update / start_angle_dma
    -> core_iic_read_dma / core_iic_abort
    -> HAL I2C DMA
```

依赖：

- [IIC FOC简化版](../../IIC/03_Firmware/FOC简化版/04_platform/platform_mcu/i2c/inc/core_iic.h)：同步寄存器读写、DMA 启动和中止
- [TIME](../../TIME/MOC.md)：`core_time_get_us()`、`core_time_get_us_delta()`
- [DWT 测量](../../DWT测量/MOC.md)：可选的 DMA 耗时统计
- 工程侧 `03_config/feature_config.h`：DWT 测量开关
- CubeMX 生成的 `i2c.h` / `main.h`：I2C 句柄和 HAL 类型
