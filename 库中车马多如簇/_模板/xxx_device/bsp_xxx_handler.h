#ifndef BSP_XXX_HANDLER_H
#define BSP_XXX_HANDLER_H

#include <stdint.h>
#include <stddef.h>
#include "bsp_xxx_driver.h"

/*
 * xxx Handler
 * 事件驱动 + 单任务串行执行，天然互斥（所有 xxx 操作经事件队列，
 * 由 xxx_handler_thread 单任务依次处理，无需 mutex）。
 * OS 能力通过函数指针接口注入，不依赖具体 OS 类型。
 */

/* Handler 状态码 */
typedef enum
{
    XXX_HANDLER_OK = 0,
    XXX_HANDLER_ERROR,
    XXX_HANDLER_ERROR_TIMEOUT,
    XXX_HANDLER_ERROR_RESOURCE,
    XXX_HANDLER_ERROR_PARAMETER,
    XXX_HANDLER_ERROR_NOMEM,
    XXX_HANDLER_ERROR_ISR,
    XXX_HANDLER_ERROR_UNKNOWN,
    XXX_HANDLER_ERROR_NOTINIT,
    XXX_HANDLER_RESERVED = 0x1FFFFFFF,
} xxx_handler_status_t;

/* xxx 事件类型 */
typedef enum
{
    XXX_HANDLER_EVENT_READ = 0,
    XXX_HANDLER_EVENT_WRITE,
    /* 按需增加：ERASE / WAKEUP / SLEEP / TEST ... */
} xxx_handler_event_type_t;

/* xxx 事件（仅 READ/WRITE 需要 addr + size + p_data，其他可传 NULL） */
typedef struct
{
    uint32_t                   addr;         /* 读/写地址 */
    uint32_t                   size;         /* 读/写长度 */
    uint8_t                    *p_data;      /* 读/写缓冲 */
    xxx_handler_event_type_t   type;         /* 事件类型 */
    void (*pf_callback)(void *);             /* 完成回调，可为 NULL */
} xxx_handler_event_t;

/* ===== OS 接口（函数指针注入）===== */

typedef struct
{
    void (*pf_os_delay)(uint32_t);
    xxx_handler_status_t (*pf_os_queue_create)(uint32_t item_num,
                                               uint32_t item_size,
                                               void **queue_handle);
    xxx_handler_status_t (*pf_os_queue_send)(void *queue_handle,
                                             void *item,
                                             uint32_t timeout);
    xxx_handler_status_t (*pf_os_queue_receive)(void *queue_handle,
                                                void *msg,
                                                uint32_t timeout);
} xxx_handler_os_interface_t;

/* ===== input_arg（实例化时注入的接口包）===== */

typedef struct
{
    xxx_bus_interface_t        *p_bus_instance;        /* 总线接口 */
    xxx_timebase_interface_t   *p_timebase_instance;   /* 时基接口 */
    xxx_handler_os_interface_t *p_os_instance;         /* OS 接口 */
    xxx_os_delay_interface_t   *p_os_delay_instance;   /* 延时接口 */
} xxx_handler_all_input_arg_t;

/* ===== Handler 实例 ===== */

typedef struct xxx_handler_private_data xxx_handler_private_data_t;

typedef struct
{
    /* 注入的接口 */
    xxx_bus_interface_t        *p_bus_instance;
    xxx_timebase_interface_t   *p_timebase_instance;
    xxx_handler_os_interface_t *p_os_instance;
    xxx_os_delay_interface_t   *p_os_delay_instance;

    /* 驱动实例 */
    xxx_driver_t               *p_xxx_instance;

    /* 事件队列句柄 */
    void                       *p_event_queue_handle;

    /* 私有数据 */
    xxx_handler_private_data_t *p_private_data;
} bsp_xxx_handler_t;

/* ===== 接口 ===== */

/* 实例化 xxx handler（注入接口 + 创建事件队列 + 初始化驱动） */
xxx_handler_status_t bsp_xxx_handler_inst(
    bsp_xxx_handler_t          *p_xxx_handler_instance,
    xxx_handler_all_input_arg_t *input_arg);

/* 处理一个 xxx 事件（分发到驱动的 pf_* 函数） */
xxx_handler_status_t xxx_event_handle(
    const bsp_xxx_handler_t   *const p_xxx_handler_instance,
    const xxx_handler_event_t *const event);

/* 把事件放入事件队列 */
xxx_handler_status_t xxx_handler_event_put(
    const xxx_handler_event_t *const event);

/* Handler 任务：从事件队列取事件并串行处理 */
void xxx_handler_thread(void *argument);

#endif /* BSP_XXX_HANDLER_H */
