#ifndef SERVICE_EXTERNFLASH_MANAGE_H
#define SERVICE_EXTERNFLASH_MANAGE_H

#include <stdint.h>

/*
 * 外部 Flash 异步读写服务
 *
 * 提供通用读写接口（绝对地址），不绑定任何业务分区：
 *   - 上层发事件到 storage_manager_task 单任务（事件组）；
 *   - storage_manager_task 串行处理，经 port 转到底层 flash 驱动；
 *   - 完成通过事件组同步。
 *
 * 分区（OTA 区 / LVGL 区等）属于上层业务，由业务层定义自己的地址范围后
 * 调用本服务的读写接口，本服务不关心分区划分。
 */

/* 事件定义（通用请求类型） */
#define EVENT_READ                      (1U << 0)
#define EVENT_WRITE                     (1U << 1)
#define EVENT_ERASE                     (1U << 2)

typedef enum
{
    Ext_Flash_OK = 0,
    Ext_Flash_ERROR = 1,
    Ext_Flash_ERRORTIMEOUT = 2,
    Ext_Flash_ERRORRESOURCE = 3,
    Ext_Flash_ERRORPARAMETER = 4,
    Ext_Flash_ERRORNOMEMORY = 5,
    Ext_Flash_RESERVED = 0x7FFFFFFF
} ext_flash_status_t;

/* 读外部 Flash（addr 为绝对地址） */
ext_flash_status_t extern_flash_read(uint32_t addr, uint32_t size, uint8_t *buf);

/* 写外部 Flash（addr 为绝对地址，内部先擦后写） */
ext_flash_status_t extern_flash_write(uint32_t addr, uint32_t size, uint8_t *buf);

/* 擦除外部 Flash 扇区（addr 为绝对地址） */
ext_flash_status_t extern_flash_erase(uint32_t addr, uint32_t size);

/* 外部 Flash 存储管理任务（单任务串行处理事件） */
void storage_manager_task(void *argument);

#endif /* SERVICE_EXTERNFLASH_MANAGE_H */
