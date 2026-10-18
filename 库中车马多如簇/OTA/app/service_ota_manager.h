/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file service_ota_manager.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief OTA 服务状态、升级标志和任务接口。
 *
 * Processing flow:
 *
 * 任务入口调用状态机，状态机通过升级标志管理镜像下载和更新。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef SERVICE_OTA_MANAGER_H
#define SERVICE_OTA_MANAGER_H

//******************************** Includes *********************************//
#include <stdint.h>
//******************************** Includes *********************************//

//******************************** Types ************************************//
/**
 * @brief OTA 服务状态。
 */
typedef enum
{
    WaitReqDownload = 0, /* 等待下载请求        */
    OtaDownload,         /* 正在接收 OTA 镜像   */
    WaitReqUpdate,       /* 等待确认安装镜像    */
    OtaEnd               /* 下载完成，准备复位  */
} E_Ota_State;

/**
 * @brief Bootloader 与 App 共享的升级标志数据。
 */
typedef struct
{
    uint32_t u32_App_RunState;      /* 当前 App 运行状态。 */
    uint32_t u32_App_RunState_Anti; /* 运行状态反码。 */
    uint32_t u32_App_AreaASize;     /* A 区镜像大小。 */
    uint32_t u32_App_AreaASize_Anti;/* A 区镜像大小反码。 */
    uint32_t u32_App_AreaBState;    /* B 区备份状态。 */
    uint32_t u32_App_AreaBState_Anti; /* B 区状态反码。 */
    uint32_t u32_App_AreaBSize;     /* B 区镜像大小。 */
    uint32_t u32_App_AreaBSize_Anti;/* B 区镜像大小反码。 */
} st_App_Info_t;

/**
 * @brief 应用标志字段。
 */
typedef enum
{
    App_RunState = 0, /* App 运行状态字段。 和下面的en_App_State_t搭配使用*/
    App_AreaASize,    /* A 区镜像大小字段。 */
    App_AreaBState,   /* B 区备份状态字段。 */
    App_AreaBSize     /* B 区镜像大小字段。 */
} en_App_FlagType_t;

/**
 * @brief 应用升级状态值。
 */
typedef enum
{
    APP_WAIT_UPDATE      = 0x11111111UL,    /* 等待安装 A 区镜像。 */
    APP_Check_NewApp     = 0x22222222UL,    /* 检查新 App 是否有效 */
    APP_AreaAToAreaB     = 0x33333333UL,    /* 执行 A 区到 B 区备份*/
    APP_Valid            = 0x44444444UL,    /* 当前 App 有效       */
    APP_AreaBState_Valid = 0x55555555UL     /* B 区备份镜像有效    */
} en_App_State_t;
//******************************** Types ************************************//

//******************************** Definitions ******************************//
#define ApplicationAddress         (0x0800C000UL) /* App 起始地址。 */
#define AppFlagAddress             (0x08008000UL) /* 升级标志存储地址。 */
#define NVIC_VectTab_FLASH         (0x08000000UL) /* 片内 Flash 向量表地址。 */
#define BOOT_MANAGER_APP_INFO_ADDR (AppFlagAddress) /* Bootloader 标志地址。 */
#define BOOT_FLAGRESULT            (0xFFFFFFFFUL) /* 无效标志默认值。 */
#define MAXAPP_SIZE                (464UL * 1024UL) /* App 最大镜像大小。 */
#define BUFFER_SIZE                (1030U) /* Ymodem 数据缓冲区大小。 */
//******************************** Definitions ******************************//

//******************************** Variables ********************************//
extern st_App_Info_t AppInfo;
extern uint32_t g_u32_datalength;
//******************************** Variables ********************************//

//******************************** Declaring ********************************//
/**
 * @brief OTA 控制任务入口。
 * @param[in] argument : 任务参数。
 * @return void : 无返回值。
 */
void ota_task_runnable(void *argument);

/**
 * @brief OTA 数据写入任务入口。
 * @param[in] argument : 任务参数。
 * @return void : 无返回值。
 */
void download_app_data_task_runnable(void *argument);

/**
 * @brief 更新一个应用升级标志
 * @param[in] AppFlagType : 应用标志类型。
 * @param[in] AppFlagValue : 应用标志值。
 * @return void : 无返回值。
 */
void set_app_flag_value(en_App_FlagType_t AppFlagType,
                        uint32_t AppFlagValue);

/**
 * @brief 读取并校验一个应用升级标志,给bootloader用的
 * @param[in] AppFlagType : 应用标志类型。
 * @param[out] p_u32_AppFlagValue : 返回应用标志值。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
uint8_t read_app_flag_value(en_App_FlagType_t AppFlagType,
                            uint32_t *p_u32_AppFlagValue);

/**
 * @brief 请求系统复位。
 * @return void : 无返回值。
 */
void SoftReset(void);
//******************************** Declaring ********************************//

#endif /* SERVICE_OTA_MANAGER_H */
