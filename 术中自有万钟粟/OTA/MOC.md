# OTA 升级

[← 返回技术栈](../MOC.md) | [← 主页](../../index.md)

OTA 笔记讲原理、流程和模式选择；可直接复制的 C 实现统一放在[OTA 库](../../库中车马多如簇/OTA/MOC.md)。

## 学习路线

1. [流程步骤](./流程步骤.md)与[环境配置](./环境配置.md)
2. [板子上的 boot0 和 boot1](./板子上的boot0和boot1.md)
3. [IAP 实操与原理](./IAP实操+原理.md)
4. [Ymodem 传输协议](./Ymodem传输协议.md)
5. [AES 加密](./AES加密.md)与[外部 Flash 备份](./外部flash备份.md)

## 三种工作模式

| 模式       | 特点                                           | Ymodem 写入区          |
| ---------- | ---------------------------------------------- | ---------------------- |
| 阻塞式下载 | BootLoader 等待串口文件，接收后直接写 APP      | `OTA_APP_ADDRESS`    |
| AES 备份式 | 下载密文、解密、安装并保留回滚备份             | `OTA_AREA_A_ADDRESS` |
| APP 后台式 | APP 下载并写状态，复位后由 BootLoader 完成安装 | `OTA_AREA_A_ADDRESS` |

## 地址模型

- A 区是下载区，下载完成后复用为旧 APP 备份区。
- B 区是解密后的新固件区。
- APP 运行区是片内 Flash 中独立的运行区域。
- A、B、APP 地址都是 OTA 逻辑地址，由 Flash Adapter 映射到实际 Backend；具体 Flash Driver 只处理 Backend 的物理地址。

## 手表式完整流程

```text
下载请求 → 下载确认 → Ymodem接收 → 双缓冲队列写A区
→ DOWNLOAD_COMPLETE → 安装确认 → INSTALL_PENDING → 复位
→ 解密并校验B区 → 备份旧APP → 安装 → 验证或回滚 → 跳转APP
```

- APP 侧由 OTA 主任务和 Flash 写任务分工，任务、两条队列和外部 Flash 互斥量统一登记到 FreeRTOS 资源表。
- 下载和安装确认使用通用回调，可由按键、LVGL、蓝牙或 UART 实现，默认超时 100 秒。
- BootLoader 通过 EEPROM 元数据在复位后继续 `INSTALL_PENDING`、`VERIFY_NEW_APP` 或 `ROLLBACK_PENDING`。
- 镜像校验和 AES 密钥来源均为 Provider；安全芯片只出现在目标工程适配层。

## 代码库入口

- [OTA 库](../../库中车马多如簇/OTA/MOC.md)
- [蓝牙透传库](../../库中车马多如簇/蓝牙bluetooth/MOC.md)(无线uart)
- [Ymodem 库](../../库中车马多如簇/Ymodem/MOC.md)
- [Flash 库](../../库中车马多如簇/Flash/MOC.md)
- [EEPROM 库](../../库中车马多如簇/EEPROM/MOC.md)
- [UART 库](../../库中车马多如簇/UART内含串口助手安装包/MOC.md)
- [AES 库](../../库中车马多如簇/AES/MOC.md)
- [看门狗库](../../库中车马多如簇/看门狗/MOC.md)

任务、队列、互斥量、信号量、软件定时器和堆内存统一从 [FreeRTOS MOC](../嵌入式工程架构/freeRTOS/MOC.md) 进入；资源用途和代码接口见 [OTA 库](../../库中车马多如簇/OTA/MOC.md)。
