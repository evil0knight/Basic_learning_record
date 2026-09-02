#ifndef BSP_WRAPPER_EEPROM_H
#define BSP_WRAPPER_EEPROM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * EEPROM Wrapper（函数表注册模式）
 *
 * 本层只做一件事：保存上层注册的 eeprom_drv_t 函数表，并把 init/read/write 等
 * 调用转发到注册的驱动。Wrapper 不包含任何器件/OS 类型，通过 bsp_wrapper_eeprom_reg
 * 注册驱动后即可使用。
 *
 * 使用方式：
 *   1. 在 Adapter 里实现 eeprom_drv_t 函数表；
 *   2. 初始化时调用 bsp_wrapper_eeprom_reg(index, &drv) 注册；
 *   3. 上层只调用 bsp_wrapper_eeprom_read/write 等接口。
 *
 * EEPROM 属 A 类（同步）外设：单次读写原子、快速，不需要后台线程，
 * 因此读写接口直接同步返回结果，无需事件和信号量。
 */

/* EEPROM 统一状态码 */
typedef enum
{
    EEPROM_OK = 0,
    EEPROM_ERROR,
    EEPROM_ERROR_TIMEOUT,
    EEPROM_ERROR_RESOURCE,
    EEPROM_ERROR_PARAMETER,
    EEPROM_ERROR_NOT_READY,
    EEPROM_RESERVED = 0x1FFFFFFF
} eeprom_status_t;

/* EEPROM 驱动函数表：成员按外设增删 */
typedef struct _eeprom_drv_t
{
    uint32_t idx;      /* 实例索引 */
    uint32_t dev_id;   /* 器件 id */
    void *user_data;   /* 用户数据 */

    eeprom_status_t (*eeprom_drv_init)(struct _eeprom_drv_t *dev);
    eeprom_status_t (*eeprom_drv_deinit)(struct _eeprom_drv_t *dev);
    eeprom_status_t (*eeprom_drv_read)(struct _eeprom_drv_t *dev,
                                       uint32_t addr, uint32_t size,
                                       uint8_t *p_buf);
    eeprom_status_t (*eeprom_drv_write)(struct _eeprom_drv_t *dev,
                                        uint32_t addr, uint32_t size,
                                        uint8_t *p_buf);
    eeprom_status_t (*eeprom_drv_erase)(struct _eeprom_drv_t *dev);
} eeprom_drv_t;

/* 注册一个 EEPROM 驱动函数表 */
bool bsp_wrapper_eeprom_reg(uint32_t index, eeprom_drv_t *dev);

eeprom_status_t bsp_wrapper_eeprom_init(void);
eeprom_status_t bsp_wrapper_eeprom_deinit(void);
eeprom_status_t bsp_wrapper_eeprom_read(uint32_t addr, uint32_t size,
                                        uint8_t *p_buf);
eeprom_status_t bsp_wrapper_eeprom_write(uint32_t addr, uint32_t size,
                                         uint8_t *p_buf);
eeprom_status_t bsp_wrapper_eeprom_erase(void);

#endif /* BSP_WRAPPER_EEPROM_H */
