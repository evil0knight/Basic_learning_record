/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file service_ota_manager.c
 *
 * @par dependencies
 * - service_ota_manager.h
 * - ota_flash_handler.h
 * - ota_config.h
 * - ymodem.h
 * - user_task_reso_config.h
 * - osal.h
 *
 * @author YA
 *
 * @brief OTA 服务状态机、升级标志和数据接收任务模板。
 *
 * Processing flow:
 *
 * 请求下载 -> 接收镜像 -> 保存镜像 -> 更新标志 -> 等待复位。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "service_ota_manager.h"

#include "ota_flash_handler.h"
#include "ota_config.h"
#include "ymodem.h"
#include "user_task_reso_config.h"
#include "osal.h"


#ifndef __WEAK
#if defined(__GNUC__)
#define __WEAK __attribute__((weak))
#else
#define __WEAK
#endif
#endif


/* Bootloader 与 App 共享的升级标志缓存。 */
st_App_Info_t AppInfo;

/* 当前队列数据块长度。 */
uint32_t g_u32_datalength = 0U;

static E_Ota_State s_ota_state = WaitReqDownload;
static uint8_t s_ymodem_buffer[BUFFER_SIZE];


/**
 * @brief 等待产品层触发一次 OTA 下载请求。
 *
 * @return int : 非 0 表示收到请求，0 表示继续等待。
 */
__WEAK int ota_download_request(void)
{
    return 0;
}


/**
 * @brief 请求系统复位。
 *
 * @return void : 无返回值。
 */
__WEAK void ota_system_reset(void)
{
}


/**
 * @brief 擦除片内 Flash 区域。
 *
 * @param[in] address : 擦除起始地址。
 * @param[in] size : 擦除长度，单位为字节。
 * @return void : 无返回值。
 */
__WEAK void ota_internal_flash_erase(uint32_t address, uint32_t size)
{
    (void)address;
    (void)size;
}


/**
 * @brief 向片内 Flash 写入数据。
 *
 * @param[in] address : 写入起始地址。
 * @param[in] data : 待读写入的数据。
 * @param[in] size : 写入长度，单位为字节。
 * @return void : 无返回值。
 */
__WEAK void ota_internal_flash_write(uint32_t address,
                                     const uint8_t *data,
                                     uint32_t size)
{
    (void)address;
    (void)data;
    (void)size;
}


/**
 * @brief 从片内 Flash 读取数据。
 *
 * @param[in] address : 读取起始地址。
 * @param[out] data : 读取数据的缓冲区。
 * @param[in] size : 读取长度，单位为字节。
 * @return void : 无返回值。
 */
__WEAK void ota_internal_flash_read(uint32_t address,
                                    uint8_t *data,
                                    uint32_t size)
{
    (void)address;
    (void)data;
    (void)size;
}


/**
 * @brief 现在是否允许开始下载或确认安装
 * 比如检查按键有没有触发升级,上位机有没有发送上机命令
 *
 * @return int : 非 0 表示收到请求，0 表示继续等待。
 */
static int ota_wait_download_request(void)
{
    return ota_download_request();
}


/**
 * @brief 更新一个应用标志并写入反码
 *        这里用反码是为了保证不会写错:
 *        读取的时候会比对一下,如果有不对应的,就说明有写错了
 * @param[in] type : 应用标志字段。
 * @param[in] value : 应用标志值。
 * @return void : 无返回值。
 */
void set_app_flag_value(en_App_FlagType_t type, uint32_t value)
{
    /* 根据字段类型更新对应的值和反码。 */
    switch (type)
    {
        case App_RunState:
            /* 保存 App 当前运行状态及其反码。 */
            AppInfo.u32_App_RunState = value;
            AppInfo.u32_App_RunState_Anti = ~value;
            break;

        case App_AreaASize:
            /* 保存外部 Flash A 区镜像大小及其反码。 */
            AppInfo.u32_App_AreaASize = value;
            AppInfo.u32_App_AreaASize_Anti = ~value;
            break;

        case App_AreaBState:
            /* 保存外部 Flash B 区状态及其反码。 */
            AppInfo.u32_App_AreaBState = value;
            AppInfo.u32_App_AreaBState_Anti = ~value;
            break;

        case App_AreaBSize:
            /* 保存外部 Flash B 区镜像大小及其反码。 */
            AppInfo.u32_App_AreaBSize = value;
            AppInfo.u32_App_AreaBSize_Anti = ~value;
            break;

        default:
            /* 未知字段不执行 Flash 写入。 */
            return;
    }

    /* 临界区内更新标志，避免写入过程中被并发访问。 */
    (void)osal_critical_enter();

    /* 先擦除旧标志，再写入新的值和反码。 */
    ota_internal_flash_erase(AppFlagAddress, sizeof(AppInfo));
    ota_internal_flash_write(AppFlagAddress,
                             (const uint8_t *)&AppInfo,
                             sizeof(AppInfo));

    /* 完成片内 Flash 操作后退出临界区。 */
    (void)osal_critical_exit();
}


/**
 * @brief 读取并校验一个应用标志
 *        接上面,读取的时候校验,防止flash写入失败(flash写多了会坏)
 *
 * @param[in] type : 应用标志字段。
 * @param[out] value : 返回应用标志值。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
uint8_t read_app_flag_value(en_App_FlagType_t type, uint32_t *value)
{
    if (NULL == value)
    {
        return 1U;
    }

    ota_internal_flash_read(AppFlagAddress,
                            (uint8_t *)&AppInfo,
                            sizeof(AppInfo));

    switch (type)
    {
        case App_RunState:
            if (AppInfo.u32_App_RunState ==
                ~AppInfo.u32_App_RunState_Anti)
            {
                *value = AppInfo.u32_App_RunState;
                return 0U;
            }
            break;

        case App_AreaASize:
            if (AppInfo.u32_App_AreaASize ==
                ~AppInfo.u32_App_AreaASize_Anti)
            {
                *value = AppInfo.u32_App_AreaASize;
                return 0U;
            }
            break;

        case App_AreaBState:
            if (AppInfo.u32_App_AreaBState ==
                ~AppInfo.u32_App_AreaBState_Anti)
            {
                *value = AppInfo.u32_App_AreaBState;
                return 0U;
            }
            break;

        case App_AreaBSize:
            if (AppInfo.u32_App_AreaBSize ==
                ~AppInfo.u32_App_AreaBSize_Anti)
            {
                *value = AppInfo.u32_App_AreaBSize;
                return 0U;
            }
            break;

        default:
            break;
    }

    return 1U;
}


/**
 * @brief 执行 OTA 下载、确认和复位状态机。
 *
 * @param[in] argument : 任务参数，模板当前未使用。
 * @return void : 无返回值。
 */
void ota_task_runnable(void *argument)
{
    int32_t image_size; /* 保存本次 Ymodem 接收的镜像大小。 */

    (void)argument; /* 当前模板不使用任务参数。 */

    for (;;)
    {
        switch (s_ota_state)
        {
            case WaitReqDownload:
                /* 等待产品层发起新的 OTA 下载请求。 */
                if (ota_wait_download_request() != 0)
                {
                    /* 收到请求后进入镜像接收状态。 */
                    s_ota_state = OtaDownload;
                }
                break;

            case OtaDownload:
                /* 清零ram缓存，准备接收新的镜像。 */
                ota_flash_init();

                /* 接收 Ymodem 镜像，数据由 OTA Flash 封装负责保存。 */
                image_size = Ymodem_Receive(s_ymodem_buffer);
                if ((image_size > 0) &&
                    (ota_flash_write_data_end() == 0U))//提交最后一个未满扇区成功
                {
                    /* 保存镜像大小，供 Bootloader 后续搬运和校验。 */
                    set_app_flag_value(App_AreaASize,
                                       (uint32_t)image_size);

                    /* 标记镜像已经下载完成，等待安装确认。 */
                    set_app_flag_value(App_RunState, APP_WAIT_UPDATE);
                    s_ota_state = WaitReqUpdate;
                }
                else
                {
                    /* 接收或尾部写入失败，回到等待下载状态。 */
                    s_ota_state = WaitReqDownload;
                }
                break;

            case WaitReqUpdate:
                /* 等待产品层确认安装刚接收的镜像。 */
                if (ota_wait_download_request() != 0)
                {
                    /* 收到确认后进入结束状态。 */
                    s_ota_state = OtaEnd;
                }
                else
                {
                    /* 未确认时回到等待下载请求状态。 */
                    s_ota_state = WaitReqDownload;
                }
                break;

            case OtaEnd:
                /* 给外部 Flash 写入和上层处理留出稳定时间。 */
                osal_task_delay(osal_ms_to_ticks(1000U));

                /* 请求系统复位，由 Bootloader 接管镜像安装。 */
                SoftReset();
                break;

            default:
                /* 状态异常时恢复到初始等待状态。 */
                s_ota_state = WaitReqDownload;
                break;
        }
    }
}


/**
 * @brief 调用产品层的系统复位实现。
 *
 * @return void : 无返回值。
 */
void SoftReset(void)
{
    ota_system_reset();
}


/**
 * @brief 从 App 数据队列取出数据并写入外部 Flash。
 *
 * @param[in] argument : 任务参数，模板当前未使用。
 * @return void : 无返回值。
 */
void download_app_data_task_runnable(void *argument)
{
    uint8_t *data;

    (void)argument;

    for (;;)
    {
        if (NULL == user_queue_get(USER_QUEUE_IDX_OTA_APP_DATA))
        {
            osal_task_delay(osal_ms_to_ticks(10U));
            continue;
        }

        if (OSAL_SUCCESS ==
            osal_queue_receive(user_queue_get(USER_QUEUE_IDX_OTA_APP_DATA),
                               &data,
                               OSAL_WAIT_FOREVER))
        {
            if ((NULL != data) &&
                (ota_flash_write_data(data, g_u32_datalength) != 0U))
            {
                s_ota_state = WaitReqDownload;
            }
        }
    }
}
