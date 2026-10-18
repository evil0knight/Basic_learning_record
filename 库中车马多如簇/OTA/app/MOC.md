# OTA App 服务

[← 返回 OTA MOC](../MOC.md) | [← 返回模块总览](../../MOC.md) | [← 主页](../../../index.md)
[← 返回 OTA 升级](../../../术中自有万钟粟/OTA/MOC.md) |

---

OTA App 部分负责设备运行期间的镜像下载、外部 Flash 缓存和升级标志管理。它只提供服务模板，片内 Flash、外部 Flash 和复位动作通过弱函数或 Flash Service 挂载到具体工程。

## 关键文件

- [service_ota_manager.h](./service_ota_manager.h) / [service_ota_manager.c](./service_ota_manager.c)：
  上面的片内flash操作可以直接看[bsp_chipflash_driver.c](..\..\Flash\bsp_storage\driver\STM32F411\bsp_chipflash_driver.c),[bsp_chipflash_driver.h](..\..\Flash\bsp_storage\driver\STM32F411\bsp_chipflash_driver.h)
  ota_wait_download_request(void)这个函数需要写一下,用来等待和同意升级申请
  set_app_flag_value这个函数写当前进度还有app大小到flash里和bootloader隔空对话
  read_app_flag_value这个函数bootloader会用到,用来读当前步骤和app信息
- [ota_flash_handler.h](./ota_flash_handler/ota_flash_handler.h) / [ota_flash_handler.c](./ota_flash_handler/ota_flash_handler.c)：
  这里对flash再次进行了一次封装,然后写flash从这里先到ram缓存,然后再到flash里
  所以这里可以引入abbuffer,缓存池等的缓存方式
- [ota_config.h](../ota_config.h)：
  OTA 镜像容量和外部 Flash A/B 区地址配置，供服务和 Bootloader 共用。
