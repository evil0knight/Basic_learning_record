# IAP 实操与原理

[← 返回 OTA](./MOC.md) | [← 主页](../../index.md)

## 向量表与跳转

BootLoader 读取 APP 向量表的第一个字作为初始 MSP，第二个字作为复位入口地址；设置 MSP、切换 `SCB->VTOR` 后调用入口即可跳转。APP 的链接起始地址、向量表地址、擦写地址必须一致。

最小原理示例（仅用于理解，最终移植代码使用 OTA 库）：

```c
typedef void (*app_entry_t)(void);

uint32_t app_msp = *(uint32_t *)OTA_APP_ADDRESS;
uint32_t app_reset = *(uint32_t *)(OTA_APP_ADDRESS + 4U);
__set_MSP(app_msp);
((app_entry_t)app_reset)();
```

## 可复制实现

- [Boot_Manager.h](../../库中车马多如簇/OTA/bootloader/Boot_Manager.h)
- [Boot_Manager.c](../../库中车马多如簇/OTA/bootloader/Boot_Manager.c)
- [OTA 库 MOC](../../库中车马多如簇/OTA/MOC.md)

BootLoader 工程直接集成 `Boot_Manager.c/.h`，由 `OTA_StateManager()` 统一处理有效性检查、升级和跳转；APP 工程使用相同的 `OTA_APP_ADDRESS` 配置向量表。

## 地址配置

以 STM32F411 为例，BootLoader 可放在 `0x08000000`，APP 逻辑起始地址为 `0x08008000`，最大长度由 `OTA_APP_MAX_SIZE` 控制。具体片内/片外映射由 Flash Adapter 决定，不能在业务代码中硬编码某个器件的地址。

