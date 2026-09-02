#include "bsp_xxx_handler.h"
#include <stdlib.h>
#include <string.h>

/*
 * xxx Handler 实现
 * 事件驱动 + 单任务串行：上层发事件到队列，xxx_handler_thread 依次处理，
 * 所有硬件访问天然串行，无需 mutex。
 */

#ifndef XXX_HANDLER_LOG
#define XXX_HANDLER_LOG(...) ((void)0)
#endif

#define XXX_HANDLER_NOT_INITIATED (0U)
#define XXX_HANDLER_INITIATED     (1U)

#define MAX_DELAY_MS (0xFFFFFFFFUL)

/* 当前只挂载一个 xxx handler 实例，多器件时再移入 per-instance 私有数据 */
static bsp_xxx_handler_t *gp_xxx_handler_instance = NULL;

struct xxx_handler_private_data
{
    uint8_t  is_initated;
    uint32_t last_handler_time;
};

static void __mount_handler(bsp_xxx_handler_t *p_instance)
{
    gp_xxx_handler_instance = p_instance;
}

/* 初始化：创建事件队列 + 实例化驱动 */
static xxx_handler_status_t bsp_xxx_handler_init(bsp_xxx_handler_t *p_instance)
{
    xxx_status_t ret;

    if (NULL == p_instance->p_os_instance->pf_os_queue_create)
    {
        return XXX_HANDLER_ERROR_RESOURCE;
    }

    ret = p_instance->p_os_instance->pf_os_queue_create(
        10, sizeof(xxx_handler_event_t),
        &(p_instance->p_event_queue_handle));
    if (XXX_HANDLER_OK != (xxx_handler_status_t)ret)
    {
        return XXX_HANDLER_ERROR_RESOURCE;
    }

    ret = xxx_inst(p_instance->p_xxx_instance,
                   p_instance->p_bus_instance,
                   p_instance->p_timebase_instance,
                   p_instance->p_os_delay_instance);
    if (XXX_OK != ret)
    {
        return XXX_HANDLER_ERROR_RESOURCE;
    }

    return XXX_HANDLER_OK;
}

/* 实例化 xxx handler */
xxx_handler_status_t bsp_xxx_handler_inst(
    bsp_xxx_handler_t           *p_xxx_handler_instance,
    xxx_handler_all_input_arg_t *input_arg)
{
    xxx_handler_status_t ret = XXX_HANDLER_ERROR_UNKNOWN;
    xxx_handler_private_data_t *p_private_data;

    if (NULL == p_xxx_handler_instance)
    {
        return XXX_HANDLER_ERROR_PARAMETER;
    }
    if (NULL == input_arg)
    {
        return XXX_HANDLER_ERROR_PARAMETER;
    }
    if (NULL == input_arg->p_timebase_instance)
    {
        return XXX_HANDLER_ERROR_PARAMETER;
    }
    if (NULL == input_arg->p_bus_instance)
    {
        return XXX_HANDLER_ERROR_PARAMETER;
    }
    if ((NULL == input_arg->p_os_instance) ||
        (NULL == input_arg->p_os_delay_instance))
    {
        return XXX_HANDLER_ERROR_PARAMETER;
    }

    p_private_data = (xxx_handler_private_data_t *)
        malloc(sizeof(xxx_handler_private_data_t));
    if (NULL == p_private_data)
    {
        return XXX_HANDLER_ERROR_RESOURCE;
    }
    memset(p_private_data, 0, sizeof(xxx_handler_private_data_t));
    p_xxx_handler_instance->p_private_data = p_private_data;

    p_xxx_handler_instance->p_bus_instance = input_arg->p_bus_instance;
    p_xxx_handler_instance->p_timebase_instance = input_arg->p_timebase_instance;
    p_xxx_handler_instance->p_os_instance = input_arg->p_os_instance;
    p_xxx_handler_instance->p_os_delay_instance = input_arg->p_os_delay_instance;

    ret = bsp_xxx_handler_init(p_xxx_handler_instance);
    if (XXX_HANDLER_OK != ret)
    {
        return XXX_HANDLER_ERROR_RESOURCE;
    }

    p_xxx_handler_instance->p_private_data->is_initated =
        XXX_HANDLER_INITIATED;
    return XXX_HANDLER_OK;
}

/* 处理一个 xxx 事件（分发到驱动的 pf_* 函数） */
xxx_handler_status_t xxx_event_handle(
    const bsp_xxx_handler_t   *const p_xxx_handler_instance,
    const xxx_handler_event_t *const event)
{
    xxx_status_t ret = XXX_OK;
    xxx_handler_status_t status = XXX_HANDLER_ERROR_UNKNOWN;

    if (NULL == event)
    {
        return XXX_HANDLER_ERROR_PARAMETER;
    }

    switch (event->type)
    {
        case XXX_HANDLER_EVENT_READ:
            ret = p_xxx_handler_instance->p_xxx_instance->pf_read(
                p_xxx_handler_instance->p_xxx_instance,
                event->p_data, event->addr, event->size);
            break;

        case XXX_HANDLER_EVENT_WRITE:
            ret = p_xxx_handler_instance->p_xxx_instance->pf_write(
                p_xxx_handler_instance->p_xxx_instance,
                event->p_data, event->addr, event->size);
            break;

        default:
            break;
    }

    status = (ret == XXX_OK) ? XXX_HANDLER_OK : XXX_HANDLER_ERROR_RESOURCE;
    if (XXX_HANDLER_OK != status)
    {
        XXX_HANDLER_LOG("Event handle fail, ret:%d\n", (int)ret);
    }
    return status;
}

/* 把事件放入事件队列 */
xxx_handler_status_t xxx_handler_event_put(const xxx_handler_event_t *event)
{
    xxx_handler_status_t status;

    if (NULL == event)
    {
        return XXX_HANDLER_ERROR_PARAMETER;
    }
    if (NULL == gp_xxx_handler_instance)
    {
        return XXX_HANDLER_ERROR_NOTINIT;
    }
    if (XXX_HANDLER_NOT_INITIATED ==
        gp_xxx_handler_instance->p_private_data->is_initated)
    {
        return XXX_HANDLER_ERROR_RESOURCE;
    }

    status = gp_xxx_handler_instance->p_os_instance->pf_os_queue_send(
        gp_xxx_handler_instance->p_event_queue_handle,
        (void *)event, MAX_DELAY_MS);
    if (status != XXX_HANDLER_OK)
    {
        return XXX_HANDLER_ERROR_RESOURCE;
    }
    return status;
}

/* Handler 任务：从事件队列取事件并串行处理 */
void xxx_handler_thread(void *argument)
{
    xxx_handler_all_input_arg_t *input_arg = NULL;
    xxx_handler_status_t status = XXX_HANDLER_ERROR_UNKNOWN;
    xxx_driver_t xxx_driver;
    bsp_xxx_handler_t xxx_handler_instance;

    if (NULL == argument)
    {
        return;
    }
    input_arg = (xxx_handler_all_input_arg_t *)argument;

    /* 检查总线临界区接口 */
    if ((NULL == input_arg->p_bus_instance->pf_critical_enter) ||
        (NULL == input_arg->p_bus_instance->pf_critical_exit))
    {
        return;
    }

    /* 实例化期间进临界区保证完整 */
    input_arg->p_bus_instance->pf_critical_enter();
    xxx_handler_instance.p_xxx_instance = &xxx_driver;
    status = bsp_xxx_handler_inst(&xxx_handler_instance, input_arg);
    input_arg->p_bus_instance->pf_critical_exit();

    if (XXX_HANDLER_OK != status)
    {
        return;
    }

    __mount_handler(&xxx_handler_instance);

    for (;;)
    {
        xxx_handler_event_t event;

        status = xxx_handler_instance.p_os_instance->pf_os_queue_receive(
            xxx_handler_instance.p_event_queue_handle,
            &event, MAX_DELAY_MS);

        if (XXX_HANDLER_OK ==
            xxx_event_handle(&xxx_handler_instance, &event))
        {
            if (NULL != event.pf_callback)
            {
                event.pf_callback(event.p_data);
            }
        }
    }
}
