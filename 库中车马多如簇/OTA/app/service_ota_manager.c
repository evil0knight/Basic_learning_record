#include "service_ota_manager.h"
#include <string.h>
#include "ota_adapter.h"
#include "ota_flash_handler.h"
#include "stm32f411_flash_handler.h"
#include "user_task_reso_config.h"
#include "osal.h"
#include "ymodem.h"
#include "service_watchdog_monitor.h"

/*
 * OTA 状态机与后台下载
 *
 * 依赖映射：
 *   - 标志区内部 Flash 擦写                        -> stm32f411_flash_handler_erase/write/read
 *   - 软复位                                       -> ota_adapter_system_reset
 *   - UI 状态/进度/确认                            -> ota_adapter_ui_*
 *   - 数据队列                                     -> user_queue_get(USER_QUEUE_IDX_OTA_APP_DATA)
 *   - 外部 Flash 互斥量                            -> user_mutex_get(USER_MUTEX_IDX_OTA_EXT_FLASH)
 *   - 下载命令接收                                 -> ota_adapter_wait_download_request
 *   - Ymodem 接收                                  -> Ymodem_ReceiveWithSink + 手动握手
 *   - 看门狗                                      -> watchdog_register/unregister
 *
 * 注：文件大小只能从 Ymodem_ReceiveWithSink 返回值拿（下载结束后），
 *     故消费者不显示实时百分比。
 */

st_App_Info_t AppInfo;
uint32_t g_u32_datalength = 0U;   /* 当前数据块长度，全局传长度（与指针队列配套） */

static E_Ota_State s_otaState = WaitReqDownload;

/* 双缓冲槽：数据队列元素是槽指针(uint8_t*)，长度走全局 g_u32_datalength */
#define OTA_DATA_SLOT_COUNT 2U
typedef struct
{
    uint8_t data[OTA_BUFFER_SIZE];
    volatile uint8_t busy;
} st_data_slot_t;

static uint8_t s_ymodem_verify_buffer[OTA_BUFFER_SIZE];
static st_data_slot_t s_slots[OTA_DATA_SLOT_COUNT];
static volatile int32_t s_writer_error = 0;
static volatile uint32_t s_written_size = 0U;

/* ===== 标志区读写（内部 Flash，反码镜像）===== */

void set_app_flag_value(en_App_FlagType_t AppFlagType, uint32_t AppFlagValue)
{
    switch (AppFlagType)
    {
        case App_RunState:
            AppInfo.u32_App_RunState = AppFlagValue;
            AppInfo.u32_App_RunState_Anti = ~AppFlagValue;
            break;
        case App_AreaASize:
            AppInfo.u32_App_AreaASize = AppFlagValue;
            AppInfo.u32_App_AreaASize_Anti = ~AppFlagValue;
            break;
        case App_AreaBState:
            AppInfo.u32_App_AreaBState = AppFlagValue;
            AppInfo.u32_App_AreaBState_Anti = ~AppFlagValue;
            break;
        case App_AreaBSize:
            AppInfo.u32_App_AreaBSize = AppFlagValue;
            AppInfo.u32_App_AreaBSize_Anti = ~AppFlagValue;
            break;
        default:
            return;
    }

    osal_critical_enter();
    if ((stm32f411_flash_handler_erase(OTA_APP_FLAG_ADDRESS, sizeof(st_App_Info_t)) != STM32F411_FLASH_OK) ||
        (stm32f411_flash_handler_write(OTA_APP_FLAG_ADDRESS, (const uint8_t *)&AppInfo,
                                       sizeof(st_App_Info_t)) != STM32F411_FLASH_OK))
    {
        osal_critical_exit();
        return;
    }
    osal_critical_exit();
}

uint8_t read_app_flag_value(en_App_FlagType_t AppFlagType, uint32_t *p_u32_AppFlagValue)
{
    uint8_t result = 1U;

    if (p_u32_AppFlagValue == NULL)
    {
        return 1U;
    }

    (void)stm32f411_flash_handler_read(OTA_APP_FLAG_ADDRESS, (uint8_t *)&AppInfo,
                                       sizeof(st_App_Info_t));

    switch (AppFlagType)
    {
        case App_RunState:
            if (AppInfo.u32_App_RunState == ~AppInfo.u32_App_RunState_Anti)
            {
                *p_u32_AppFlagValue = AppInfo.u32_App_RunState;
                result = 0U;
            }
            break;
        case App_AreaASize:
            if (AppInfo.u32_App_AreaASize == ~AppInfo.u32_App_AreaASize_Anti)
            {
                *p_u32_AppFlagValue = AppInfo.u32_App_AreaASize;
                result = 0U;
            }
            break;
        case App_AreaBState:
            if (AppInfo.u32_App_AreaBState == ~AppInfo.u32_App_AreaBState_Anti)
            {
                *p_u32_AppFlagValue = AppInfo.u32_App_AreaBState;
                result = 0U;
            }
            break;
        case App_AreaBSize:
            if (AppInfo.u32_App_AreaBSize == ~AppInfo.u32_App_AreaBSize_Anti)
            {
                *p_u32_AppFlagValue = AppInfo.u32_App_AreaBSize;
                result = 0U;
            }
            break;
        default:
            break;
    }

    return result;
}

/* ===== 下载 Sink：复制到双缓冲槽，发槽指针，再握手等消费者写完 ===== */

static int32_t ymodem_queue_sink(const uint8_t *data, uint32_t size,
                                 uint32_t address, void *context)
{
    uint8_t *slot_data;
    uint32_t slot_index;
    (void)address;
    (void)context;

    if ((data == NULL) || (size == 0U) || (size > OTA_BUFFER_SIZE))
        return -1;

    for (;;)
    {
        if (s_writer_error != 0) return -1;
        for (slot_index = 0U; slot_index < OTA_DATA_SLOT_COUNT; ++slot_index)
            if (s_slots[slot_index].busy == 0U) break;
        if (slot_index < OTA_DATA_SLOT_COUNT) break;
        osal_task_delay(osal_ms_to_ticks(1U));
    }

    s_slots[slot_index].busy = 1U;
    memcpy(s_slots[slot_index].data, data, size);
    g_u32_datalength = size;                    /* 全局传长度 */
    slot_data = s_slots[slot_index].data;

    if (osal_queue_send(user_queue_get(USER_QUEUE_IDX_OTA_APP_DATA), &slot_data,
                        OSAL_WAIT_FOREVER) != OSAL_SUCCESS)
    {
        s_slots[slot_index].busy = 0U;
        return -1;
    }

    /* 握手：消费者(高优先级)写 Flash 时持互斥量，此处阻塞直到写完。 */
    (void)osal_mutex_take(user_mutex_get(USER_MUTEX_IDX_OTA_EXT_FLASH), OSAL_WAIT_FOREVER);
    (void)osal_mutex_give(user_mutex_get(USER_MUTEX_IDX_OTA_EXT_FLASH));

    return 0;
}

/* ===== 确认等待 ===== */

static int wait_confirmation(void)
{
    uint32_t confirmCount = 0U;

    while (ota_adapter_ui_confirm_read() == 0U)
    {
        osal_task_delay(osal_ms_to_ticks(50U));
        confirmCount++;
        if (confirmCount >= 2000U)  /* 100 秒超时 */
        {
            break;
        }
    }
    return (ota_adapter_ui_confirm_read() == 1U) ? 1 : 0;
}

/* ===== OTA 控制任务（状态机）===== */

void ota_task_runnable(void *argument)
{
    int32_t appDataLength = 0;
    (void)argument;

    for (;;)
    {
        switch (s_otaState)
        {
            case WaitReqDownload:
            {
                ota_adapter_ui_state_write(0U); /* 空闲 */

                /* 等命令是长时间阻塞，先注销看门狗避免被误判卡死 */
                watchdog_unregister(osal_task_get_current());

                if (ota_adapter_wait_download_request() != 0)
                {
                    /* 收到命令，重新注册看门狗（600 秒超时） */
                    watchdog_register(osal_task_get_current(), 600000U, "OTA_task");

                    ota_adapter_ui_state_write(1U); /* 请求下载确认 */
                    if (wait_confirmation() == 1)
                    {
                        s_otaState = OtaDownload;
                        ota_adapter_ui_state_write(0U);
                    }
                    else
                    {
                        s_otaState = WaitReqDownload;
                    }
                }
                break;
            }

            case OtaDownload:
            {
                ota_adapter_ui_progress_write(0U);
                W25Q64_Init();
                appDataLength = Ymodem_ReceiveWithSink(s_ymodem_verify_buffer,
                                                       ymodem_queue_sink, NULL,
                                                       OTA_AREA_A_ADDRESS);
                ota_adapter_ui_progress_write(100U);

                if ((appDataLength > 0) && (s_writer_error == 0))
                {
                    s_otaState = WaitReqUpdate;
                    if (W25Q64_WriteData_End(BLOCKA) != 0U)
                    {
                        s_otaState = WaitReqDownload;
                        break;
                    }
                    set_app_flag_value(App_AreaASize, (uint32_t)appDataLength);
                    set_app_flag_value(App_RunState, APP_WAIT_UPDATE);
                }
                else
                {
                    s_otaState = WaitReqDownload;
                }
                break;
            }

            case WaitReqUpdate:
            {
                ota_adapter_ui_state_write(2U); /* 下载完成待安装 */
                ota_adapter_ui_confirm_write(0U);

                if (wait_confirmation() == 1)
                {
                    s_otaState = OtaEnd;
                }
                else
                {
                    s_otaState = WaitReqDownload;
                }
                break;
            }

            case OtaEnd:
            {
                osal_task_delay(osal_ms_to_ticks(1000U));
                SoftReset();
                break;
            }

            default:
                break;
        }
    }
}

/* OTA 完成后触发系统复位 */
void SoftReset(void)
{
    ota_adapter_system_reset();
}

/* ===== 下载数据写任务（消费者）===== */

void download_app_data_task_runnable(void *argument)
{
    uint8_t *pData = NULL;
    uint32_t slot_index;
    (void)argument;

    for (;;)
    {
        if (user_queue_get(USER_QUEUE_IDX_OTA_APP_DATA) == NULL)
        {
            osal_task_delay(osal_ms_to_ticks(10U));
            continue;
        }

        if (osal_queue_receive(user_queue_get(USER_QUEUE_IDX_OTA_APP_DATA), &pData,
                               OSAL_WAIT_FOREVER) != OSAL_SUCCESS)
        {
            continue;
        }

        if (pData == NULL)
        {
            s_writer_error = 1;
            continue;
        }

        /* 写 Flash 前注册看门狗（5 秒超时），写完注销 */
        watchdog_register(osal_task_get_current(), 5000U, "DwAppData_task");

        /* 持互斥量写外部 Flash（串行化） */
        if (osal_mutex_take(user_mutex_get(USER_MUTEX_IDX_OTA_EXT_FLASH), OSAL_WAIT_FOREVER) == OSAL_SUCCESS)
        {
            if (W25Q64_WriteData(BLOCKA, pData, g_u32_datalength) == 0U)
            {
                s_written_size += g_u32_datalength;
            }
            else
            {
                s_writer_error = 1;
            }
            osal_mutex_give(user_mutex_get(USER_MUTEX_IDX_OTA_EXT_FLASH));
        }
        else
        {
            s_writer_error = 1;
        }

        watchdog_unregister(osal_task_get_current());

        /* 释放对应的双缓冲槽 */
        for (slot_index = 0U; slot_index < OTA_DATA_SLOT_COUNT; ++slot_index)
        {
            if (pData == s_slots[slot_index].data)
            {
                s_slots[slot_index].busy = 0U;
                break;
            }
        }
    }
}
