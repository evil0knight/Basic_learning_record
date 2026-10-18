/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file service_externflash_manage.c
 *
 * @par dependencies
 * - service_externflash_manage.h
 * - externflash_manage_port.h
 * - bsp_adapter_port_flash.h
 * - osal.h
 * - user_task_reso_config.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 外部 Flash 异步读写服务实现（事件组 + 单任务串行）。 *
 * Processing flow:
 *
 * extern_flash_read/write/erase (绝对地址)
 *   -> 填充共享参数并置位事件位
 *   -> storage_manager_task 等待事件位
 *   -> 分发到 port -> 底层驱动
 *   -> 驱动完成回调 give 信号量
 *   -> service 等到信号量后置位 DONE 事件, 唤醒原调用方。
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes*********************************//

#include "service_externflash_manage.h"

#include "externflash_manage_port.h"
#include "bsp_adapter_port_flash.h"
#include "osal.h"
#include "user_task_reso_config.h"
#include <stddef.h>

//********************************Includes*********************************//


#define EXT_FLASH_EVENT_DONE (1U << 4)

/* 服务层共享状态事件句柄 + 本次请求。addr / size / buf / 结果 */
static osal_event_handle_t s_extFlashEvent  = NULL;

static uint32_t            s_targetAddress  = 0U;
static uint32_t            s_transferSize   = 0U;
static uint8_t            *s_pData          = NULL;
static volatile ext_flash_status_t s_requestStatus = Ext_Flash_OK;

/**
 * @brief 清空驱动完成信号残留 *
 * Steps:
 *  1. 取外。Flash 信号量句柄 *  2. 0 超时反复 take, 直到计数清零 *
 * @return void */
static void extern_flash_drain_driver_completion(void)
{
    osal_sema_handle_t semaHandle = user_sema_get(USER_SEMA_IDX_EXT_FLASH);

    /* 句柄不存在视为无残留, 直接返回 */
    if (NULL != semaHandle)
    {
        /* 0 超时 take, 把计数耗干 */
        while (OSAL_SUCCESS == osal_sema_take(semaHandle, 0U))
        {
        }
    }
}

/**
 * @brief 阻塞等待驱动完成（adapter 完成回调 give 信号量后返回） *
 * @return ext_flash_status_t : 状态码 */
static ext_flash_status_t extern_flash_wait_driver_completion(void)
{
    osal_sema_handle_t semaHandle = user_sema_get(USER_SEMA_IDX_EXT_FLASH);

    /* 资源缺失直接报错 */
    if (NULL == semaHandle)
    {
        return Ext_Flash_ERRORRESOURCE;
    }

    /* 无限阻塞等待驱动回调 give */
    if (OSAL_SUCCESS != osal_sema_take(semaHandle, OSAL_WAIT_FOREVER))
    {
        return Ext_Flash_ERROR;
    }

    return Ext_Flash_OK;
}

/**
 * @brief 置位请求事件位并阻塞等待 DONE。
 *
 * Steps:
 *  1. 预置失败态状态字, 清掉上次 DONE。
 *  2. 置位请求事件。 唤醒 storage_manager_task。
 *  3. 等待 DONE 事件。 读取结果状态字
 * @param[in] eventBit : 请求事件位（EVENT_READ/WRITE/ERASE 之一）
 * @return ext_flash_status_t : 结果状态。 */
static ext_flash_status_t extern_flash_notify_and_wait(uint32_t eventBit)
{
    /* 事件组未就绪, 不能服务*/
    if (NULL == s_extFlashEvent)
    {
        return Ext_Flash_ERRORRESOURCE;
    }

    /* 预置失败状态,等任务处理完会覆盖为真实结果 */
    s_requestStatus = Ext_Flash_ERROR;
    (void)osal_event_group_clear_bits(s_extFlashEvent, EXT_FLASH_EVENT_DONE);

    /* 置位请求标志位,任务被唤醒后开始处理*/
    (void)osal_event_group_set_bits(s_extFlashEvent, eventBit);

    /* 等待任务置位 DONE */
    if (OSAL_SUCCESS != osal_event_group_wait_bits(s_extFlashEvent,
                                                   EXT_FLASH_EVENT_DONE,
                                                   true, false,
                                                   OSAL_WAIT_FOREVER, NULL))
    {
        return Ext_Flash_ERRORTIMEOUT;
    }

    return s_requestStatus;
}

/**
 * @brief 置位请求完成事件位,唤醒等待方
 *        就是等待方调用这些函数是用事件驱动的
 * @param[in] status : 本次处理结果
 * @return void
 */
static void extern_flash_signal_request_completion(ext_flash_status_t status)
{
    s_requestStatus = status;

    if (NULL != s_extFlashEvent)
    {
        (void)osal_event_group_set_bits(s_extFlashEvent, EXT_FLASH_EVENT_DONE);
    }
}

/**
 * @brief 同步读外部Flash（绝对地址）
 * @param[in]  addr : 读起始地址
 * @param[in]  size : 读取长度
 * @param[out] buf  : 读缓冲
 * @return ext_flash_status_t : 状态码
 */
ext_flash_status_t extern_flash_read(uint32_t addr, uint32_t size,
                                     uint8_t *buf)
{
    /* 参数检查。*/
    if ((NULL == buf) || (0U == size))
    {
        return Ext_Flash_ERRORPARAMETER;
    }

    /* 填入共享参数, 发事件等结果 */
    s_targetAddress = addr;
    s_transferSize  = size;
    s_pData         = buf;

    //这里通知task读数据,读到buf里去
    return extern_flash_notify_and_wait(EVENT_READ);
}

/**
 * @brief 同步写外部Flash（绝对地址, 内部先擦后写）。
 * @param[in] addr : 写起始地址。
 * @param[in] size : 写入长度。
 * @param[in] buf  : 待写数据。
 * @return ext_flash_status_t : 状态码
 */
ext_flash_status_t extern_flash_write(uint32_t addr, uint32_t size,
                                      uint8_t *buf)
{
    if ((NULL == buf) || (0U == size))
    {
        return Ext_Flash_ERRORPARAMETER;
    }

    s_targetAddress = addr;
    s_transferSize  = size;
    s_pData         = buf;

    //这里通知task写数据,把buf里的数据写到flash
    return extern_flash_notify_and_wait(EVENT_WRITE);
}

/**
 * @brief 同步擦除外部 Flash 一段区域（按4KB 对齐循环擦除）。
 * @param[in] addr : 起始地址。
 * @param[in] size : 擦除字节数。
 * @return ext_flash_status_t : 状态码。
 */
ext_flash_status_t extern_flash_erase(uint32_t addr, uint32_t size)
{
    if (0U == size)
    {
        return Ext_Flash_ERRORPARAMETER;
    }

    s_targetAddress = addr;
    s_transferSize  = size;
    s_pData         = NULL;

    //这里通知task擦除flash数据
    return extern_flash_notify_and_wait(EVENT_ERASE);
}

/**
 * @brief 外部 Flash 存储管理任务: 单任务串行处理读/写擦事件。
 * Steps:
 *  1. 首次运行时懒创建事件组。
 *  2. 死循环等待事件位, 根据触发事件分发到对应 port 调用。
 *  3. 读写 drain -> 发起 -> 等驱动完成 -> 置位 DONE;
 *     擦除。4KB 步进循环, 任一步失败即跳出, 最终置位DONE。
 * @param[in] argument : 任务参数（未使用）。
 * @return void。
 */
void storage_manager_task(void *argument)
{
    uint32_t eventBitsToWaitFor = EVENT_READ | EVENT_WRITE | EVENT_ERASE;

    (void)argument;

    /* 懒创建事件组, 避免外部初始化顺序依赖*/
    if (NULL == s_extFlashEvent)
    {
        (void)osal_event_group_create(&s_extFlashEvent);//创建事件组
    }

    for (;;)
    {
        uint32_t activeBits = 0U;

        /* 阻塞等待任一请求。*/
        if (OSAL_SUCCESS != osal_event_group_wait_bits(s_extFlashEvent,
                                                       eventBitsToWaitFor,
                                                       true, false,
                                                       OSAL_WAIT_FOREVER,
                                                       &activeBits))
        {
            continue;
        }

        /* 根据实际触发的事件位分发 */
        switch (activeBits & eventBitsToWaitFor)
        {
            case EVENT_READ:
                /* 清残留-> 发起读-> 等驱动回调-> 置位 DONE */
                extern_flash_drain_driver_completion();
                externflash_read(s_targetAddress, s_transferSize, s_pData);
                //先阻塞等待,然后下层的信号量回调通知,信号量到了再用事件通知上层
                extern_flash_signal_request_completion(
                    extern_flash_wait_driver_completion());
                break;

            case EVENT_WRITE:
                /* 清残留 -> 发起写 -> 等驱动回调 -> 置位 DONE */
                extern_flash_drain_driver_completion();
                externflash_write(s_targetAddress, s_transferSize, s_pData);
                //先阻塞等待,然后下层的信号量回调通知,信号量到了再用事件通知上层
                extern_flash_signal_request_completion(
                    extern_flash_wait_driver_completion());
                break;

            case EVENT_ERASE:
            {
                uint32_t offset;

                /* 按 4KB 步进循环擦除, 任一步失败立刻跳出 */
                for (offset = 0U; offset < s_transferSize; offset += 4096U)
                {
                    extern_flash_drain_driver_completion();
                    externflash_erasesector(s_targetAddress + offset);
                    if (Ext_Flash_OK != extern_flash_wait_driver_completion())
                    {//这里等底层的信号量
                        break;
                    }
                }
                //信号量全都没问题,上层事件组通知
                extern_flash_signal_request_completion((offset >= s_transferSize) ? Ext_Flash_OK : Ext_Flash_ERROR);
                break;
            }

            default:
                /* 不应到达 */
                break;
        }
    }
}
