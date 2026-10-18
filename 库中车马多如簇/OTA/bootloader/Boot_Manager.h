/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file Boot_Manager.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief Bootloader OTA 状态机接口。
 *
 * Processing flow:
 *
 * Boot_Manager_State -> 应用标志判断 -> 镜像搬运、回滚或跳转 App。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef BOOT_MANAGER_H
#define BOOT_MANAGER_H

//******************************** Includes *********************************//
#include <stdint.h>
//******************************** Includes *********************************//

//******************************** Types ************************************//
typedef void (*pFunction)(void);
//******************************** Types ************************************//

//******************************** Declaring ********************************//
/**
 * @brief 检查应用标志并跳转到 App。
 * @param[in] None : 无输入参数。
 * @return void : 无返回值。
 */
void BootManager_JumpToApp(void);

/**
 * @brief 检查 App 镜像长度和向量表栈顶地址。
 * @param[in] file_size : 镜像长度，单位为字节。
 * @return int8_t : 0 表示有效，-1 表示无效。
 */
int8_t CheckAppValid(int32_t file_size);

/**
 * @brief 从外部 Flash 读取镜像并写入 App 区。
 * @param[in] block_index : 外部 Flash 分区索引。
 * @return int8_t : 0 表示成功，-1 表示失败。
 */
int8_t BootManager_BackToApp(uint8_t block_index);

/**
 * @brief 初始化升级校验使用的 AES 环境。
 * @param[in] None : 无输入参数。
 * @return int8_t : 0 表示成功，非 0 表示失败。
 */
int8_t InitAESKeyAndIV(void);

/**
 * @brief 通过 Ymodem 接收镜像并保存到外部 Flash。
 * @param[in] None : 无输入参数。
 * @return void : 无返回值。
 */
void BootManager_BootDownload(void);

/**
 * @brief 执行 Bootloader 上电升级状态机。
 * @param[in] None : 无输入参数。
 * @return void : 无返回值。
 */
void Boot_Manager_State(void);

/**
 * @brief 将 A 区镜像复制到 B 区备份区。
 * @param[in] None : 无输入参数。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
uint8_t Boot_Manager_AareaToBarea(void);

/**
 * @brief 处理等待安装的新 App 镜像。
 * @param[in] None : 无输入参数。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
uint8_t Boot_Manager_AppWaitUpdate(void);

/**
 * @brief 从 B 区备份镜像执行回滚。
 * @param[in] None : 无输入参数。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
uint8_t Boot_Manager_Rollback_Backup_Area(void);
//******************************** Declaring ********************************//

#endif /* BOOT_MANAGER_H */

