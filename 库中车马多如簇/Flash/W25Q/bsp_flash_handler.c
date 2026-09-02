#include "bsp_flash_handler.h"
#include <stdlib.h>
#include <string.h>

/*
 * W25Q64 Flash Handler 实现（照手表 bsp_flash_handler.c 搬运）
 * 事件驱动 + 单任务串行：上层发事件到队列，flash_handler_thread 依次处理，
 * 所有 flash 访问天然串行，无需 mutex。
 * 看门狗通过宏适配，默认不喂（产品层可覆盖宏绑定看门狗）。
 */

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

/* 当前只挂载一个 flash handler 实例，多器件时再移入 per-instance 私有数据 */
static bsp_flash_handler_t *gp_flash_handler_instance = NULL;

typedef struct flash_handler_private_data
{
    uint8_t  is_initated;
    uint32_t last_handler_time;
} flash_handler_private_data_t;

static void __mount_handler(bsp_flash_handler_t *p_instance)
{
    gp_flash_handler_instance = p_instance;
}

/* 初始化：创建事件队列 + 实例化驱动 */
static flash_handler_status_t bsp_flash_handler_init(bsp_flash_handler_t *p_instance)
{
    flash_handler_status_t ret;

    if (NULL == p_instance->p_os_instance->pf_os_queue_create)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }

    ret = p_instance->p_os_instance->pf_os_queue_create(
        10, sizeof(flash_handler_event_t),
        &(p_instance->p_event_queue_handle));
    if (FLASH_HANDLER_OK != ret)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }

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

/* 实例化 flash handler */
flash_handler_status_t bsp_flash_handler_inst(
    bsp_flash_handler_t           *p_flash_handler_instance,
    flash_handler_all_input_arg_t *input_arg)
{
    flash_handler_status_t ret = FLASH_HANDLER_ERROR_UNKNOWN;
    flash_handler_private_data_t *p_private_data;

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

    p_private_data = (flash_handler_private_data_t *)
        malloc(sizeof(flash_handler_private_data_t));
    if (NULL == p_private_data)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }
    memset(p_private_data, 0, sizeof(flash_handler_private_data_t));
    p_flash_handler_instance->p_private_data = p_private_data;

    p_flash_handler_instance->p_spi_w25q64_instance =
        input_arg->p_spi_w25q64_instance;
    p_flash_handler_instance->p_timebase_instance =
        input_arg->p_timebase_instance;
    p_flash_handler_instance->p_os_instance =
        input_arg->p_os_instance;
    p_flash_handler_instance->p_os_delay_instance =
        input_arg->p_os_delay_instance;

    ret = bsp_flash_handler_init(p_flash_handler_instance);
    if (FLASH_HANDLER_OK != ret)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }

    p_flash_handler_instance->p_private_data->is_initated =
        FLASH_HANDLER_INITIATED;
    return FLASH_HANDLER_OK;
}

/* 处理一个 flash 事件（分发到驱动的 pf_* 函数） */
flash_handler_status_t flash_event_handle(
    const bsp_flash_handler_t   *const p_flash_handler_instance,
    const flash_handler_event_t *const event)
{
    w25q64_status_t ret = W25Q64_OK;
    flash_handler_status_t status = FLASH_HANDLER_ERROR_UNKNOWN;

    if (NULL == event)
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }

    /* 睡眠状态先唤醒 */
    if ((W25Q64_SLEEP ==
         read_w25qxx_isSleep(p_flash_handler_instance->p_w25q64_instance)) &&
        (FLASH_HANDLER_EVENT_SLEEP != event->type))
    {
        p_flash_handler_instance->p_w25q64_instance->pf_wakeup(
            p_flash_handler_instance->p_w25q64_instance);
    }

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

        case FLASH_HANDLER_EVENT_SLEEP:
            ret = p_flash_handler_instance->p_w25q64_instance->pf_sleep(
                p_flash_handler_instance->p_w25q64_instance);
            break;

        default:
            break;
    }

    status = (ret == W25Q64_OK) ? FLASH_HANDLER_OK : FLASH_HANDLER_ERROR_RESOURCE;
    if (FLASH_HANDLER_OK != status)
    {
        FLASH_HANDLER_LOG("Event handle fail, ret:%d\n", (int)ret);
    }
    return status;
}

/* 把事件放入事件队列 */
flash_handler_status_t flash_handler_event_put(
    const flash_handler_event_t *const event)
{
    flash_handler_status_t status;

    if (NULL == event)
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }
    if (NULL == gp_flash_handler_instance)
    {
        return FLASH_HANDLER_ERROR_NOTINIT;
    }
    if (FLASH_HANDLER_EVENT_TEST == event->type)
    {
        return FLASH_HANDLER_OK;
    }
    if (FLASH_HANDLER_NOT_INITIATED ==
        gp_flash_handler_instance->p_private_data->is_initated)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }

    status = gp_flash_handler_instance->p_os_instance->pf_os_queue_send(
        gp_flash_handler_instance->p_event_queue_handle,
        (void *)event, MAX_DELAY_MS);
    if (status != FLASH_HANDLER_OK)
    {
        return FLASH_HANDLER_ERROR_RESOURCE;
    }
    return status;
}

/* Handler 任务：从事件队列取事件并串行处理 */
void flash_handler_thread(void *argument)
{
    flash_handler_all_input_arg_t *input_arg = NULL;
    flash_handler_status_t status = FLASH_HANDLER_ERROR_UNKNOWN;
    bsp_w25q64_driver_t bsp_w25q64_driver;
    bsp_flash_handler_t flash_handler_instance;

    if (NULL == argument)
    {
        return;
    }
    input_arg = (flash_handler_all_input_arg_t *)argument;

    /* 检查 SPI 临界区接口 */
    if ((NULL == input_arg->p_spi_w25q64_instance->pf_critical_enter) ||
        (NULL == input_arg->p_spi_w25q64_instance->pf_critical_exit))
    {
        return;
    }

    /* 实例化期间进临界区保证完整 */
    input_arg->p_spi_w25q64_instance->pf_critical_enter();
    flash_handler_instance.p_w25q64_instance = &bsp_w25q64_driver;
    status = bsp_flash_handler_inst(&flash_handler_instance, input_arg);
    input_arg->p_spi_w25q64_instance->pf_critical_exit();

    if (FLASH_HANDLER_OK != status)
    {
        return;
    }

    __mount_handler(&flash_handler_instance);

    for (;;)
    {
        flash_handler_event_t event;

        status = flash_handler_instance.p_os_instance->pf_os_queue_receive(
            flash_handler_instance.p_event_queue_handle,
            &event, MAX_DELAY_MS);

        FLASH_HANDLER_WATCHDOG_REGISTER();

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
