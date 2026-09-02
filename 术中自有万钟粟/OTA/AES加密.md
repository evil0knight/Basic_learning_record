# AES-CBC 加密

[← 返回 OTA](./MOC.md) | [← 主页](../../index.md)

OTA 使用 AES-CBC 保护传输固件。密钥和 IV 由产品安全配置提供，明文长度按 16 字节对齐并采用约定的填充方式；加密不等于完整性校验，仍需校验长度、CRC 或签名。

## OTA 解密流程

1. 上位机对固件分块加密并生成镜像头。
2. Ymodem 将密文写入逻辑 A 区。
3. BootLoader 从 A 区读取密文，使用同一 IV/密钥逐块解密到逻辑 B 区。
4. 校验解密镜像后，将 B 区安装到片内 APP 运行区；失败时保留 A 区旧 APP 备份并回滚。

## 可复制实现

- [AES MOC](../../库中车马多如簇/AES/MOC.md)
- [AES.c](../../库中车马多如簇/AES/AES.c) / [AES.h](../../库中车马多如簇/AES/AES.h)
- [Boot_Manager.c](../../库中车马多如簇/OTA/bootloader/Boot_Manager.c) / [Boot_Manager.h](../../库中车马多如簇/OTA/bootloader/Boot_Manager.h)

AES 库提供 AES-128/192/256 的 CBC 接口。BootLoader 不搬运 AES 实现，只在安装流程中调用库接口；具体密钥、IV 和镜像头布局统一配置在[ota_config.h](../../库中车马多如簇/OTA/common/ota_config.h)。
