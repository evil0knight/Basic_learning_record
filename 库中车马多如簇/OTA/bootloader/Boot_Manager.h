#ifndef BOOT_MANAGER_H
#define BOOT_MANAGER_H

#include <stdint.h>
#include "service_ota_manager.h"

/* BootLoader 升级状态机，成功时跳转到 APP */
void Boot_Manager_State(void);

/* 跳转到 APP（向量表/栈/复位向量） */
void BootManager_JumpToApp(void);

/* BootLoader 内直接 Ymodem 接收下载 -> 解密搬运 -> 跳转（无有效 App 时用） */
void BootManager_BootDownload(void);

#endif /* BOOT_MANAGER_H */
