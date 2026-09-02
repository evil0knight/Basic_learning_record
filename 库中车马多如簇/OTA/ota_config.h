#ifndef OTA_CONFIG_H
#define OTA_CONFIG_H

#include <stdint.h>

/*
 * OTA 地址配置
 *
 * 本文件是 OTA 唯一需要按产品/芯片修改的"数据"配置，集中管理所有地址和尺寸。
 * 与 [ota_adapter.h](ota_adapter.h) 的分工：
 *   - ota_config.h：只配"地址/尺寸"这类常量；
 *   - ota_adapter.c：实现"内部 Flash 擦写/复位/跳转/UI/资源"这类操作。
 *
 * 换产品：改本文件地址 + 实现 ota_adapter.c 的 UI/资源/命令接收。
 * 换芯片：改本文件地址 + 重写 ota_adapter.c 的内部 Flash/复位/跳转。
 *
 * 地址分两类，注意区分：
 *   1) 内部 Flash（MCU 片内，存 bootloader/标志位/APP）：
 *        | 0x08000000            | bootloader    | OTA_BOOTLOADER_ADDRESS
 *        | 0x08008000            | 标志位        | OTA_APP_FLAG_ADDRESS
 *        | 0x0800C000            | APP 运行区    | OTA_APP_ADDRESS
 *   2) 外部 Flash（W25Q64，存固件镜像，逻辑地址）：
 *        | 0x00000000            | A 区(下载/旧APP备份) | OTA_AREA_A_ADDRESS
 *        | 0x00080000            | B 区(备份)          | OTA_AREA_B_ADDRESS
 */

/* ===================== 内部 Flash 地址 ===================== */

/*
 * BootLoader 起始地址（内部 Flash）
 * bootloader 从内部 Flash 起始处烧录，上电先运行它，由它读标志位决定跳转/升级。
 * 换芯片或调整 bootloader 大小时改这里。
 */
#define OTA_BOOTLOADER_ADDRESS      0x08000000UL

/*
 * APP 运行区地址（内部 Flash）
 * 解密/搬运后的新固件最终写入这里，bootloader 校验通过后跳转到此地址执行。
 */
#define OTA_APP_ADDRESS             0x0800C000UL

/*
 * 标志位地址（内部 Flash）
 * 存 st_App_Info_t 结构（含反码镜像），记录升级状态机执行到哪一步，
 * APP 端和 BootLoader 端共用同一份，上电时由 BootLoader 读它续跑状态机。
 */
#define OTA_APP_FLAG_ADDRESS        0x08008000UL

/* ===================== 外部 Flash 地址 ===================== */

/*
 * A 区：下载区（外部 Flash，逻辑地址）
 * APP 端下载的密文固件先写到这里；升级成功后再复用为"旧 APP 备份区"，
 * 升级失败时从 B 区回退。地址是外部 Flash 内的逻辑偏移，实际物理地址由
 * Flash Wrapper 映射。
 */
#define OTA_AREA_A_ADDRESS          0x00000000UL
#define OTA_AREA_A_SIZE             0x00080000UL   /* A 区容量 512KB */

/*
 * B 区：备份区（外部 Flash，逻辑地址）
 * 升级成功确认后，把 A 区（新固件）备份到 B 区；若后续升级失败，从 B 区
 * 解密回滚到 APP 区。紧跟在 A 区之后。
 */
#define OTA_AREA_B_ADDRESS          0x00080000UL
#define OTA_AREA_B_SIZE             0x00080000UL   /* B 区容量 512KB */

/* ===================== 尺寸 ===================== */

/*
 * APP 允许的最大字节数（含固件头和尾部校验）
 * 下载/解密/回退时都用它做边界校验，超出即判失败。
 */
#define OTA_APP_MAX_SIZE            (464UL * 1024UL)

/*
 * 首次升级时用于备份的原 APP 长度
 * 第一次升级前还没有标志位记录旧 APP 实际长度，用它兜底；
 * 后续升级以标志位里记录的 AreaASize 为准。
 */
#define OTA_INITIAL_APP_SIZE        OTA_APP_MAX_SIZE

/*
 * Ymodem 接收缓冲区大小（字节）
 * Ymodem 一个 1KB 数据包 + 包头/包序/CRC 开销，所以比 1024 略大。
 */
#define OTA_BUFFER_SIZE             (1030U)

/* ===================== AES 密钥材料 ===================== */

/*
 * AES 块大小 / IV 长度（字节），AES 固定为 16。
 */
#define OTA_AES_BLOCK_SIZE          16U

/*
 * AES-256 密钥长度（字节）。
 */
#define OTA_AES_KEY_SIZE            32U

/*
 * AES-256 密钥（32 字节），量产必须替换。
 * 默认全 0 占位；若用安全芯片方案，此宏可忽略，
 * 改在 ota_adapter.c 的 ota_adapter_aes_key_get() 里从 KEROS 等安全芯片读取。
 */
#define OTA_AES_KEY_BYTES           {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, \
                                     0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, \
                                     0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, \
                                     0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}

/*
 * AES IV（16 字节），量产必须替换。
 */
#define OTA_AES_IV_BYTES            {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, \
                                     0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}

#endif /* OTA_CONFIG_H */
