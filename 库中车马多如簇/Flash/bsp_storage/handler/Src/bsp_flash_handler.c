/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_flash_handler.c
 *
 * @par dependencies
 * - bsp_flash_handler.h
 * - stdlib.h
 * - string.h
 *
 * @author YA（从手表工程搬运）。 *
 * @brief W25Q64 Flash Handler 实现（事件驱动 + 单任务串行）。 *
 * Processing flow:
 *
 * 上层投递事件 -> flash_handler_thread 阻塞 queue_receive ->
 * 睡眠态则先唤醒 -> switch 分发到驱动的 pf_* 函数 ->
 * 完成后回调 event.pf_callback。 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//********************************Includes*********************************//

#include "bsp_flash_handler.h"

#include <stdlib.h>
#include <string.h>

//********************************Includes*********************************//


#ifndef FLASH_HANDLER_LOG
#define FLASH_HANDLER_LOG(...) ((void)0)
#endif
#ifndef FLASH_HANDLER_WATCHDOG_REGISTER
#define FLASH_HANDLER_WATCHDOG_REGISTER() ((void)0)
#endif
#ifndef FLASH_HANDLER_WATCHDOG_UNREGISTER
#define FLASH_HANDLER_WATCHDOG_UNREGISTER() ((void)0)
#endif

#define FLASH_HANDLER_NOT_INITIATED (0U)
#define FLASH_HANDLER_INITIATED     (1U)

#define MAX_DELAY_MS (0xFFFFFFFFUL)

// 注册Handler 实例,用于管理多个driver
static bsp_flash_handler_t *gp_flash_handler_instance = NULL;

/* Handler 私有数据 */
typedef struct flash_handler_private_data
{
    uint8_t  is_initated;         /* 初始化标。*/
    uint32_t last_handler_time;   /* 最近一次处理时间（预留)*/
} flash_handler_private_data_t;

/**
 * @brief 挂载当前 Handler 实例指针（供 flash_handler_event_put 使用）
 * @param[in] p_instance : Handler 实例
 * */
static void __mount_handler(bsp_flash_handler_t *p_instance)
{
    gp_flash_handler_instance = p_instance;
}

/**
 * @brief 初始化 Handler 底层资源和 W25Q64 Driver
 * Steps:
 *  1. 检查 OS 队列创建接口
 *  2. 创建深度为 10 的事件队列
 *  3. 设置队列消息大小为 flash_handler_event_t
 *  4. 调用 w25q64_inst() 实例化 W25Q64 Driver
 * @param[in] p_instance : Handler 实例。
 * @return flash_handler_status_t : 状态码。
 */
static flash_handler_status_t bsp_flash_handler_init(
    bsp_flash_handler_t *p_instance)
{
    flash_handler_status_t ret;

    /* 必备接口检查*/
    if (NULL == p_instance->p_os_instance->pf_os_queue_create)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }

    /* 创建事件队列: 深度 10 */
    ret = p_instance->p_os_instance->pf_os_queue_create(
        10, sizeof(flash_handler_event_t),
        &(p_instance->p_event_queue_handle));
    if (FLASH_HANDLER_OK != ret)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }

    /* 实例化 W25Q64 驱动 */
    ret = w25q64_inst(p_instance->p_w25q64_instance,
                      p_instance->p_spi_w25q64_instance,
                      p_instance->p_timebase_instance,
                      p_instance->p_os_delay_instance);
    if (W25Q64_OK != ret)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }

    return FLASH_HANDLER_OK;
}

/**
 * @brief 实例化 Flash Handler
 * Steps:
 *  1. 检查 Handler、输入参数及各接口指针
 *  2. 分配并清零 Handler 私有数据
 *  3. 保存外部 SPI、时基、OS 和延时接口
 *     (input_arg是adapter层挂载成一个flash_handler_all_input_arg,
 *      然后在user_init.c中创建任务并将其注入到任务中,任务中再懒初始化使用OS资源)
 *  4. 调用 bsp_flash_handler_init() 完成底层初始化
 *  5. 设置 Handler 初始化完成标志
 * @param[in] p_flash_handler_instance : Handler 实例。
 * @param[in] input_arg                : 外部接口参数包。
 * @return flash_handler_status_t      : 状态码。
 */
flash_handler_status_t bsp_flash_handler_inst(
    bsp_flash_handler_t           *p_flash_handler_instance,
    flash_handler_all_input_arg_t *input_arg)
{
    flash_handler_status_t ret = FLASH_HANDLER_ERROR_UNKNOWN;
    flash_handler_private_data_t *p_private_data;

    /* 参数检查。*/
    if (NULL == p_flash_handler_instance)
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }
    if (NULL == input_arg)
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }
    if (NULL == input_arg->p_timebase_instance)
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }
    if (NULL == input_arg->p_spi_w25q64_instance)
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }
    if ((NULL == input_arg->p_os_instance) ||
        (NULL == input_arg->p_os_delay_instance))
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }

    /* 分配并清零私有数。*/
    p_private_data = (flash_handler_private_data_t *)
        malloc(sizeof(flash_handler_private_data_t));
    if (NULL == p_private_data)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }
    memset(p_private_data, 0, sizeof(flash_handler_private_data_t));
    p_flash_handler_instance->p_private_data = p_private_data;

    /* 挂接外部接口 */
    p_flash_handler_instance->p_spi_w25q64_instance =
        input_arg->p_spi_w25q64_instance;
    p_flash_handler_instance->p_timebase_instance =
        input_arg->p_timebase_instance;
    p_flash_handler_instance->p_os_instance =
        input_arg->p_os_instance;
    p_flash_handler_instance->p_os_delay_instance =
        input_arg->p_os_delay_instance;

    /* 底层初始化。*/
    ret = bsp_flash_handler_init(p_flash_handler_instance);
    if (FLASH_HANDLER_OK != ret)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }

    /* 置初始化完成标志 */
    p_flash_handler_instance->p_private_data->is_initated =
        FLASH_HANDLER_INITIATED;
    return FLASH_HANDLER_OK;
}

/**
 * @brief 处理一个 Flash 事件并分发至 W25Q64 Driver。
 * Steps:
 *  1. 检查事件参数。
 *  2. 芯片处于睡眠状态且当前事件不是 SLEEP 时，先执行唤醒。
 *  3. 根据事件类型调用对应的 Driver 操作函数。
 *  4. 将 Driver 返回值转换为 Handler 状态码。
 * @param[in] p_flash_handler_instance : Handler 实例。
 * @param[in] event                    : Flash 事件。
 * @return flash_handler_status_t      : Handler 状态码。
 */
flash_handler_status_t flash_event_handle(
    const bsp_flash_handler_t   *const p_flash_handler_instance,
    const flash_handler_event_t *const event)
{
    w25q64_status_t ret           = W25Q64_OK;
    flash_handler_status_t status = FLASH_HANDLER_ERROR_UNKNOWN;

    /* 参数检查。*/
    if (NULL == event)
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }

    /* 睡眠态先唤醒(SLEEP 事件除外)*/
    if ((W25Q64_SLEEP ==
         read_w25qxx_isSleep(p_flash_handler_instance->p_w25q64_instance)) &&
        (FLASH_HANDLER_EVENT_SLEEP != event->type))
    {
        p_flash_handler_instance->p_w25q64_instance->pf_wakeup(
            p_flash_handler_instance->p_w25q64_instance);
    }

    /* 事件分发 */
    switch (event->type)
    {
        case FLASH_HANDLER_EVENT_READ:
            ret = p_flash_handler_instance->p_w25q64_instance->pf_read(
                p_flash_handler_instance->p_w25q64_instance,
                event->p_data, event->addr, event->size);
            break;

        case FLASH_HANDLER_EVENT_WRITE:
            ret = p_flash_handler_instance->p_w25q64_instance->pf_write_erase(
                p_flash_handler_instance->p_w25q64_instance,
                event->p_data, event->addr, event->size);
            break;

        case FLASH_HANDLER_EVENT_WRITE_NOERASE:
            ret = p_flash_handler_instance->p_w25q64_instance->pf_write_no_erase(
                p_flash_handler_instance->p_w25q64_instance,
                event->p_data, event->addr, event->size);
            break;

        case FLASH_HANDLER_EVENT_ERASE_CHIP:
            ret = p_flash_handler_instance->p_w25q64_instance->pf_erase_chip(
                p_flash_handler_instance->p_w25q64_instance);
            break;

        case FLASH_HANDLER_EVENT_ERASE_SECTOR:
            ret = p_flash_handler_instance->p_w25q64_instance->pf_erase_sector(
                p_flash_handler_instance->p_w25q64_instance, event->addr);
            break;

        case FLASH_HANDLER_EVENT_WAKEUP:
            ret = p_flash_handler_instance->p_w25q64_instance->pf_wakeup(
                p_flash_handler_instance->p_w25q64_instance);
            break;

        case FLASH_HANDLER_EVENT_SLEEP:
            ret = p_flash_handler_instance->p_w25q64_instance->pf_sleep(
                p_flash_handler_instance->p_w25q64_instance);
            break;

        default:
            /* 其余类型暂不处理 */
            break;
    }

    /* 转状态码 */
    status = (W25Q64_OK == ret)
                 ? FLASH_HANDLER_OK : FLASH_HANDLER_ERROR_RESOURCE;
    if (FLASH_HANDLER_OK != status)
    {
        FLASH_HANDLER_LOG("Event handle fail, ret:%d\n", (int)ret);
    }
    return status;
}

/** 
 * @brief 投递事件到事件队列,通知handler去处理
 *
 * @param[in] event : 事件内容。
 * @return flash_handler_status_t : 状态码
 */
flash_handler_status_t flash_handler_event_put(
    const flash_handler_event_t *const event)
{
    flash_handler_status_t status;

    /* 参数检查。*/
    if (NULL == event)
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }
    if (NULL == gp_flash_handler_instance)
    {
        return FLASH_HANDLER_ERROR_NOTINIT;
    }

    /* TEST 事件直接返回成功 */
    if (FLASH_HANDLER_EVENT_TEST == event->type)
    {
        return FLASH_HANDLER_OK;
    }

    /* 初始化状态检查。*/
    if (FLASH_HANDLER_NOT_INITIATED ==
        gp_flash_handler_instance->p_private_data->is_initated)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }

    /* 入队 */
    status = gp_flash_handler_instance->p_os_instance->pf_os_queue_send(
        gp_flash_handler_instance->p_event_queue_handle,
        (void *)event, MAX_DELAY_MS);
    if (FLASH_HANDLER_OK != status)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }
    return status;
}

/**
 * @brief Handler 任务：串行处理 Flash 事件。
 * Steps:
 *  1. 检查任务参数和临界区接口。
 *  2. 在临界区内实例化 Handler 和 W25Q64 Driver。
 *  3. 初始化成功后，将 Handler 挂载到全局指针。
 *  4. 循环接收队列事件，调用处理函数并触发完成回调。
 * @param[in] argument : flash_handler_all_input_arg_t 接口参数指针。
 * @return void : 无返回值。
 */
void flash_handler_thread(void *argument)
{
    flash_handler_all_input_arg_t *input_arg = NULL;
    flash_handler_status_t status            = FLASH_HANDLER_ERROR_UNKNOWN;
    bsp_w25q64_driver_t     bsp_w25q64_driver;
    bsp_flash_handler_t     flash_handler_instance;

    /* 参数检查。*/
    if (NULL == argument)
    {
        return;
    }
    input_arg = (flash_handler_all_input_arg_t *)argument;

    /* 临界区接口必须就。*/
    if ((NULL == input_arg->p_spi_w25q64_instance->pf_critical_enter) ||
        (NULL == input_arg->p_spi_w25q64_instance->pf_critical_exit))
    {
        return;
    }

    /* 实例化期间整体临界区: 防止初始化被打断 */
    input_arg->p_spi_w25q64_instance->pf_critical_enter();
    flash_handler_instance.p_w25q64_instance = &bsp_w25q64_driver;
    status = bsp_flash_handler_inst(&flash_handler_instance, input_arg);
    input_arg->p_spi_w25q64_instance->pf_critical_exit();

    if (FLASH_HANDLER_OK != status)
    {
        return;
    }

    /* 实例挂到全局, 之后 flash_handler_event_put 才可。*/
    __mount_handler(&flash_handler_instance);

    for (;;)
    {
        flash_handler_event_t event;

        /* 阻塞取事。*/
        status = flash_handler_instance.p_os_instance->pf_os_queue_receive(
            flash_handler_instance.p_event_queue_handle,
            &event, MAX_DELAY_MS);

        /* 处理期间注册看门狗(避免 Flash 擦写阻塞复位)*/
        FLASH_HANDLER_WATCHDOG_REGISTER();

        /* 分发事件, 成功则触发回。*/
        if (FLASH_HANDLER_OK ==
            flash_event_handle(&flash_handler_instance, &event))
        {
            if (NULL != event.pf_callback)
            {
                event.pf_callback(event.p_data);
            }
        }

        FLASH_HANDLER_WATCHDOG_UNREGISTER();
    }
}
