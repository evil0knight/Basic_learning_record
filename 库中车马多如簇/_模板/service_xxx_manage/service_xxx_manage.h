#ifndef SERVICE_XXX_MANAGE_H
#define SERVICE_XXX_MANAGE_H

#include <stdint.h>

/*
 * xxx 异步读写服务
 *
 * 提供通用读写接口（绝对地址 / 业务参数），不绑定任何业务分区：
 *   - 上层发事件到 xxx_manager_task 单任务（事件组）；
 *   - xxx_manager_task 串行处理，经 port 转到底层 xxx 驱动；
 *   - 完成通过事件组同步。
 *
 * 分区 / 业务含义属于上层业务，由业务层定义后调用本服务的读写接口，
 * 本服务不关心分区划分。
 */

/* 事件定义（通用请求类型，按外设增删） */
#define EVENT_XXX_READ   (1U << 0)
#define EVENT_XXX_WRITE  (1U << 1)
#define EVENT_XXX_ERASE  (1U << 2)

typedef enum
{
    XXX_SERVICE_OK = 0,
    XXX_SERVICE_ERROR = 1,
    XXX_SERVICE_ERRORTIMEOUT = 2,
    XXX_SERVICE_ERRORRESOURCE = 3,
    XXX_SERVICE_ERRORPARAMETER = 4,
    XXX_SERVICE_ERRORNOMEMORY = 5,
    XXX_SERVICE_RESERVED = 0x7FFFFFFF
} xxx_service_status_t;

/* 读 xxx（addr 为绝对地址 / 业务参数） */
xxx_service_status_t xxx_read(uint32_t addr, uint32_t size, uint8_t *buf);

/* 写 xxx（addr 为绝对地址 / 业务参数） */
xxx_service_status_t xxx_write(uint32_t addr, uint32_t size, uint8_t *buf);

/* 擦除 xxx（按需） */
xxx_service_status_t xxx_erase(uint32_t addr, uint32_t size);

/* xxx 管理任务（单任务串行处理事件） */
void xxx_manager_task(void *argument);

#endif /* SERVICE_XXX_MANAGE_H */
