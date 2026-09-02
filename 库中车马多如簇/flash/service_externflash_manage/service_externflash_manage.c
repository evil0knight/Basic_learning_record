#include "service_externflash_manage.h"
#include "externflash_manage_port.h"
#include "bsp_adapter_port_flash.h"
#include "osal.h"
#include "user_task_reso_config.h"
#include <stddef.h>

/*
 * 外部 Flash 异步读写服务实现
 *
 * 上层调 extern_flash_read/write/erase（绝对地址）→ 发事件到事件组 →
 * storage_manager_task 单任务串行处理 → 经 port 转到底层驱动 → 完成置位事件组。
 * 不绑定任何业务分区，分区地址由调用方（业务层）传入。
 */

#define EXT_FLASH_EVENT_DONE (1U << 4)

static osal_event_handle_t s_extFlashEvent = NULL;
static uint32_t s_targetAddress = 0U;
static uint32_t s_transferSize = 0U;
static uint8_t *s_pData = NULL;
static volatile ext_flash_status_t s_requestStatus = Ext_Flash_OK;

/* 清除残留的驱动完成信号 */
static void extern_flash_drain_driver_completion(void)
{
    osal_sema_handle_t semaHandle = user_sema_get(USER_SEMA_IDX_EXT_FLASH);
    if (semaHandle != NULL)
    {
        while (osal_sema_take(semaHandle, 0U) == OSAL_SUCCESS)
        {
        }
    }
}

/* 等待驱动完成（adapter 完成回调 give 信号量） */
static ext_flash_status_t extern_flash_wait_driver_completion(void)
{
    osal_sema_handle_t semaHandle = user_sema_get(USER_SEMA_IDX_EXT_FLASH);
    if (semaHandle == NULL)
    {
        return Ext_Flash_ERRORRESOURCE;
    }
    if (osal_sema_take(semaHandle, OSAL_WAIT_FOREVER) != OSAL_SUCCESS)
    {
        return Ext_Flash_ERROR;
    }
    return Ext_Flash_OK;
}

/* 发事件 + 等完成（事件组） */
static ext_flash_status_t extern_flash_notify_and_wait(uint32_t eventBit)
{
    if (s_extFlashEvent == NULL)
    {
        return Ext_Flash_ERRORRESOURCE;
    }

    s_requestStatus = Ext_Flash_ERROR;
    (void)osal_event_group_clear_bits(s_extFlashEvent, EXT_FLASH_EVENT_DONE);
    (void)osal_event_group_set_bits(s_extFlashEvent, eventBit);
    if (osal_event_group_wait_bits(s_extFlashEvent, EXT_FLASH_EVENT_DONE,
                                   true, false, OSAL_WAIT_FOREVER, NULL) != OSAL_SUCCESS)
    {
        return Ext_Flash_ERRORTIMEOUT;
    }
    return s_requestStatus;
}

/* 置位完成事件 */
static void extern_flash_signal_request_completion(ext_flash_status_t status)
{
    s_requestStatus = status;
    if (s_extFlashEvent != NULL)
    {
        (void)osal_event_group_set_bits(s_extFlashEvent, EXT_FLASH_EVENT_DONE);
    }
}

/* 读外部 Flash（绝对地址） */
ext_flash_status_t extern_flash_read(uint32_t addr, uint32_t size, uint8_t *buf)
{
    if ((buf == NULL) || (size == 0U))
    {
        return Ext_Flash_ERRORPARAMETER;
    }
    s_targetAddress = addr;
    s_transferSize = size;
    s_pData = buf;
    return extern_flash_notify_and_wait(EVENT_READ);
}

/* 写外部 Flash（绝对地址，内部先擦后写） */
ext_flash_status_t extern_flash_write(uint32_t addr, uint32_t size, uint8_t *buf)
{
    if ((buf == NULL) || (size == 0U))
    {
        return Ext_Flash_ERRORPARAMETER;
    }
    s_targetAddress = addr;
    s_transferSize = size;
    s_pData = buf;
    return extern_flash_notify_and_wait(EVENT_WRITE);
}

/* 擦除外部 Flash 扇区（绝对地址，按 4KB 对齐循环擦除） */
ext_flash_status_t extern_flash_erase(uint32_t addr, uint32_t size)
{
    if (size == 0U)
    {
        return Ext_Flash_ERRORPARAMETER;
    }
    s_targetAddress = addr;
    s_transferSize = size;
    s_pData = NULL;
    return extern_flash_notify_and_wait(EVENT_ERASE);
}

/* 外部 Flash 存储管理任务：单任务串行处理事件 */
void storage_manager_task(void *argument)
{
    uint32_t eventBitsToWaitFor = EVENT_READ | EVENT_WRITE | EVENT_ERASE;
    (void)argument;

    if (s_extFlashEvent == NULL)
    {
        (void)osal_event_group_create(&s_extFlashEvent);
    }

    for (;;)
    {
        uint32_t activeBits = 0U;

        if (osal_event_group_wait_bits(s_extFlashEvent, eventBitsToWaitFor,
                                       true, false, OSAL_WAIT_FOREVER,
                                       &activeBits) != OSAL_SUCCESS)
        {
            continue;
        }

        switch (activeBits & eventBitsToWaitFor)
        {
            case EVENT_READ:
                extern_flash_drain_driver_completion();
                externflash_read(s_targetAddress, s_transferSize, s_pData);
                extern_flash_signal_request_completion(
                    extern_flash_wait_driver_completion());
                break;

            case EVENT_WRITE:
                extern_flash_drain_driver_completion();
                externflash_write(s_targetAddress, s_transferSize, s_pData);
                extern_flash_signal_request_completion(
                    extern_flash_wait_driver_completion());
                break;

            case EVENT_ERASE:
            {
                uint32_t offset;
                for (offset = 0U; offset < s_transferSize; offset += 4096U)
                {
                    extern_flash_drain_driver_completion();
                    externflash_erasesector(s_targetAddress + offset);
                    if (extern_flash_wait_driver_completion() != Ext_Flash_OK)
                    {
                        break;
                    }
                }
                extern_flash_signal_request_completion(Ext_Flash_OK);
                break;
            }

            default:
                break;
        }
    }
}
