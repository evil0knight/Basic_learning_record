#ifndef SERVICE_OTA_MANAGER_H
#define SERVICE_OTA_MANAGER_H

#include <stdint.h>
#include "ota_config.h"

/* ===== 状态机状态 ===== */
typedef enum
{
    WaitReqDownload = 0,
    OtaDownload,
    WaitReqUpdate,
    OtaEnd
} E_Ota_State;

/* ===== 标志区结构（内部 Flash，反码镜像）===== */
typedef struct
{
    uint32_t u32_App_RunState;
    uint32_t u32_App_RunState_Anti;
    uint32_t u32_App_AreaASize;
    uint32_t u32_App_AreaASize_Anti;
    uint32_t u32_App_AreaBState;
    uint32_t u32_App_AreaBState_Anti;
    uint32_t u32_App_AreaBSize;
    uint32_t u32_App_AreaBSize_Anti;
} st_App_Info_t;

typedef enum
{
    App_RunState = 0,
    App_AreaASize,
    App_AreaBState,
    App_AreaBSize
} en_App_FlagType_t;

typedef enum
{
    APP_WAIT_UPDATE   = 0x11111111UL,
    APP_Check_NewApp  = 0x22222222UL,
    APP_AreaAToAreaB  = 0x33333333UL,
    APP_Valid         = 0x44444444UL,
    APP_AreaBState_Valid = 0x55555555UL,
} en_App_State_t;

extern st_App_Info_t AppInfo;
extern uint32_t g_u32_datalength;

/* 运行 OTA 控制任务（状态机） */
void ota_task_runnable(void *argument);

/* 运行下载数据写任务（消费者） */
void download_app_data_task_runnable(void *argument);

/* 更新标志区一个字段（含反码镜像） */
void set_app_flag_value(en_App_FlagType_t AppFlagType, uint32_t AppFlagValue);

/* 读取并校验标志区一个字段 */
uint8_t read_app_flag_value(en_App_FlagType_t AppFlagType, uint32_t *p_u32_AppFlagValue);

/* OTA 完成后触发系统复位 */
void SoftReset(void);

#endif /* SERVICE_OTA_MANAGER_H */
