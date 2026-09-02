# OTA Flash 备份

[← 返回 OTA](./MOC.md) | [← 主页](../../index.md)

## A/B 逻辑区

- 新固件先下载到逻辑 A 区；BootLoader 解密后写入逻辑 B 区。
- B 区校验通过后，A 区擦除并复用为旧 APP 备份区。
- 最终由 B 区安装到片内 APP 运行区；安装失败时从 A 区恢复旧 APP。

A、B 和 APP 是 OTA 逻辑地址，Flash Wrapper/Adapter 根据逻辑地址选择片内或片外 Backend，器件 Driver 由对应 Backend 实现。

## 配置与实现入口

- [Flash MOC](../../库中车马多如簇/Flash/MOC.md)
- [flash_config.h](../../库中车马多如簇/Flash/flash_config.h)
- [Flash Wrapper](../../库中车马多如簇/Flash/bsp_wrapper_flash/bsp_wrapper_flash.c)
- [Flash Adapter](../../库中车马多如簇/Flash/bsp_adapter_port_flash/bsp_adapter_port_flash.c)
- [W25Q Driver/Handler](../../库中车马多如簇/Flash/W25Q/MOC.md)
- [STM32F411 Driver/Handler](../../库中车马多如簇/Flash/STM32F411/MOC.md)
- [OTA 配置](../../库中车马多如簇/OTA/common/ota_config.h)

在 `flash_config.h` 配置 Backend、实例、总线槽位和逻辑地址映射；W25Q、STM32F411 等具体 Driver/Handler 只作为 Backend 实现接入。

示例中的地址是 OTA 逻辑地址：

```c
#define OTA_AREA_A_ADDRESS  0x00000000UL
#define OTA_AREA_B_ADDRESS  0x00080000UL
#define OTA_APP_ADDRESS     0x08008000UL
```
