#include "service_xxx_manage.h"
#include "xxx_manage_port.h"
#include "bsp_adapter_port_xxx.h"
#include "osal.h"
#include "user_task_reso_config.h"
#include <stddef.h>

/*
 * xxx 异步读写服务实现
 *
 * 上层调 xxx_read/write/erase（绝对地址 / 业务参数）→ 发事件到事件组 →
 * xxx_manager_task 单任务串行处理 → 经 port 转到底层驱动 → 完成置位事件组。
 * 不绑定任何业务分区，分区地址由调用方（业务层）传入。
 */

#define XXX_EVENT_DONE (1U << 4)

static osal_event_handle_t s_xxxEvent = NULL;
static uint32_t s_targetAddress = 0U;
static uint32_t s_transferSize = 0U;
static uint8_t *s_pData = NULL;
static volatile xxx_service_status_t s_requestStatus = XXX_SERVICE_OK;

/* 清除残留的驱动完成信号 */
static void xxx_drain_driver_completion(void)
{
    osal_sema_handle_t semaHandle = user_sema_get(USER_SEMA_IDX_XXX);
    if (semaHandle != NULL)
    {
        while (osal_sema_take(semaHandle, 0U) == OSAL_SUCCESS)
        {
        }
    }
}

/* 等待驱动完成（adapter 完成回调 give 信号量） */
static xxx_service_status_t xxx_wait_driver_completion(void)
{
    osal_sema_handle_t semaHandle = user_sema_get(USER_SEMA_IDX_XXX);
    if (semaHandle == NULL)
    {
        return XXX_SERVICE_ERRORRESOURCE;
    }
    if (osal_sema_take(semaHandle, OSAL_WAIT_FOREVER) != OSAL_SUCCESS)
    {
        return XXX_SERVICE_ERROR;
    }
    return XXX_SERVICE_OK;
}

/* 发事件 + 等完成（事件组） */
static xxx_service_status_t xxx_notify_and_wait(uint32_t eventBit)
{
    if (s_xxxEvent == NULL)
    {
        return XXX_SERVICE_ERRORRESOURCE;
    }

    s_requestStatus = XXX_SERVICE_ERROR;
    (void)osal_event_group_clear_bits(s_xxxEvent, XXX_EVENT_DONE);
    (void)osal_event_group_set_bits(s_xxxEvent, eventBit);
    if (osal_event_group_wait_bits(s_xxxEvent, XXX_EVENT_DONE,
                                   true, false, OSAL_WAIT_FOREVER, NULL) != OSAL_SUCCESS)
    {
        return XXX_SERVICE_ERRORTIMEOUT;
    }
    return s_requestStatus;
}

/* 置位完成事件 */
static void xxx_signal_request_completion(xxx_service_status_t status)
{
    s_requestStatus = status;
    if (s_xxxEvent != NULL)
    {
        (void)osal_event_group_set_bits(s_xxxEvent, XXX_EVENT_DONE);
    }
}

/* 读 xxx */
xxx_service_status_t xxx_read(uint32_t addr, uint32_t size, uint8_t *buf)
{
    if ((buf == NULL) || (size == 0U))
    {
        return XXX_SERVICE_ERRORPARAMETER;
    }
    s_targetAddress = addr;
    s_transferSize = size;
    s_pData = buf;
    return xxx_notify_and_wait(EVENT_XXX_READ);
}

/* 写 xxx */
xxx_service_status_t xxx_write(uint32_t addr, uint32_t size, uint8_t *buf)
{
    if ((buf == NULL) || (size == 0U))
    {
        return XXX_SERVICE_ERRORPARAMETER;
    }
    s_targetAddress = addr;
    s_transferSize = size;
    s_pData = buf;
    return xxx_notify_and_wait(EVENT_XXX_WRITE);
}

/* 擦除 xxx（按需） */
xxx_service_status_t xxx_erase(uint32_t addr, uint32_t size)
{
    if (size == 0U)
    {
        return XXX_SERVICE_ERRORPARAMETER;
    }
    s_targetAddress = addr;
    s_transferSize = size;
    s_pData = NULL;
    return xxx_notify_and_wait(EVENT_XXX_ERASE);
}

/* xxx 管理任务：单任务串行处理事件 */
void xxx_manager_task(void *argument)
{
    uint32_t eventBitsToWaitFor = EVENT_XXX_READ | EVENT_XXX_WRITE | EVENT_XXX_ERASE;
    (void)argument;

    if (s_xxxEvent == NULL)
    {
        (void)osal_event_group_create(&s_xxxEvent);
    }

    for (;;)
    {
        uint32_t activeBits = 0U;

        if (osal_event_group_wait_bits(s_xxxEvent, eventBitsToWaitFor,
                                       true, false, OSAL_WAIT_FOREVER,
                                       &activeBits) != OSAL_SUCCESS)
        {
            continue;
        }

        switch (activeBits & eventBitsToWaitFor)
        {
            case EVENT_XXX_READ:
                xxx_drain_driver_completion();
                xxx_port_read(s_targetAddress, s_transferSize, s_pData);
                xxx_signal_request_completion(xxx_wait_driver_completion());
                break;

            case EVENT_XXX_WRITE:
                xxx_drain_driver_completion();
                xxx_port_write(s_targetAddress, s_transferSize, s_pData);
                xxx_signal_request_completion(xxx_wait_driver_completion());
                break;

            case EVENT_XXX_ERASE:
                /* 按外设实现擦除循环 */
                xxx_signal_request_completion(XXX_SERVICE_OK);
                break;

            default:
                break;
        }
    }
}
