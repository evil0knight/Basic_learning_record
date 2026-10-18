/******************************************************************************
 * @file ota_config.h
 * @brief OTA 地址、容量和加密参数配置。
 *****************************************************************************/
#ifndef OTA_CONFIG_H
#define OTA_CONFIG_H

#include <stdint.h>

/** @brief 片内 Flash 地址布局。 */
#define OTA_BOOTLOADER_ADDRESS (0x08000000UL)
#define OTA_APP_FLAG_ADDRESS   (0x08008000UL)
#define OTA_APP_ADDRESS        (0x0800C000UL)

/** @brief 外部 Flash A/B 分区地址和容量。 */
#define OTA_AREA_A_ADDRESS    (0x00000000UL)
#define OTA_AREA_A_SIZE       (0x00080000UL)
#define OTA_AREA_B_ADDRESS    (0x00080000UL)
#define OTA_AREA_B_SIZE       (0x00080000UL)

/** @brief OTA 镜像和 Ymodem 接收缓冲区大小。 */
#define OTA_APP_MAX_SIZE      (464UL * 1024UL)
#define OTA_INITIAL_APP_SIZE  OTA_APP_MAX_SIZE
#define OTA_BUFFER_SIZE       (1030U)

/** @brief AES 参数长度和模板默认值。 */
#define OTA_AES_BLOCK_SIZE    (16U)
#define OTA_AES_KEY_SIZE      (32U)
#define OTA_AES_KEY_BYTES     {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, \
                               0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, \
                               0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, \
                               0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}
#define OTA_AES_IV_BYTES      {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, \
                               0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}

#endif /* OTA_CONFIG_H */

