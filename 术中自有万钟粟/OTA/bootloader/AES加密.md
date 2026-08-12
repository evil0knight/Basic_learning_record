# AES 加密 — OTA 固件保护

[← bootloader](./MOC.md) | [← 主页](../../../index.md)

> AES-128-CBC：OTA 场景最常用的对称加密方案，128 位密钥 + 16 字节 IV，固件按 16 字节分块加密。
>
> 意义:防止升级失败导致系统变砖,代码回滚
>
> 1. 新固件通过OTA加密下载到非活动区
> 2. *验证新固件的完整性和有效性（如CRC校验、AES解密）*
> 3. 若验证通过，数据解密后搬运到活动区

---

## 备份:

[Ymodem传输协议](./Ymodem传输协议.md)的基础上,做出如下修改:

1. 在 `Boot_Manager.h`新增

   ```c
   /* APP_BACK：0x08020000~0x0803FFFF，占用 Sector 5，共 128 KiB。 */
   #define APP_BACK_START_ADDRESS  APP_RUN_END_ADDRESS
   #define APP_BACK_END_ADDRESS    0x08040000U
   #define APP_BACK_SIZE           (APP_BACK_END_ADDRESS - APP_BACK_START_ADDRESS)
   ```
2. `ymodem.c` 需要 `Ctrl+F`查找并 `APP_RUN_START_ADDRESS`替换 `APP_BACK_START_ADDRESS`

   接收文件大小仍使用 *APP_RUN_SIZE* 限制。*APP_BACK* 虽占用 128 KiB，但固件解密后必须能放入 96 KiB 的 *APP_RUN*。
3.
