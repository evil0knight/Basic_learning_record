#ifndef BSP_XXX_DRIVER_H
#define BSP_XXX_DRIVER_H

#include <stdint.h>
#include "bsp_xxx_reg.h"

/*
 * xxx 驱动层
 * 依赖全部通过函数指针接口注入，不直接依赖 OS / 总线类型：
 *   - xxx_bus_interface_t：总线逐字节接口（由 Adapter 用总线 Port 实现）
 *   - xxx_timebase_interface_t：时基（pf_tick_count_get）
 *   - xxx_os_delay_interface_t：延时（pf_os_delay_ms）
 */

/* xxx 状态返回码 */
typedef enum
{
    XXX_OK = 0,
    XXX_ERROR,
    XXX_ERROR_TIMEOUT,
    XXX_ERROR_RESOURCE,
    XXX_ERROR_PARAMETER,
    XXX_ERROR_NOMEM,
    XXX_BUSY,
    XXX_SLEEP,
    XXX_WAKEUP,
    XXX_ERROR_ISR,
    XXX_ERROR_UNKNOWN,
    XXX_RESERVED = 0x1FFFFFFF,
} xxx_status_t;

/* ===== 总线逐字节接口（由 Adapter 适配注入）===== */

typedef struct
{
    void    (*pf_bus_init)(void *);          /* 总线初始化 */
    void    (*pf_bus_deinit)(void *);        /* 总线去初始化 */
    uint8_t (*pf_bus_cs_set)(void);          /* 片选/起始 */
    uint8_t (*pf_bus_cs_deset)(void);        /* 片选释放/停止 */
    uint8_t (*pf_bus_write_byte)(uint8_t);   /* 写一个字节 */
    uint8_t (*pf_bus_read_byte)(void);       /* 读一个字节 */

    /* OS 临界区（保证传输期间数据完整） */
    void    (*pf_critical_enter)(void);
    void    (*pf_critical_exit)(void);
} xxx_bus_interface_t;

/* ===== 时基接口 ===== */

typedef struct
{
    uint32_t (*pf_tick_count_get)(void);  /* 获取 tick */
} xxx_timebase_interface_t;

/* ===== OS 延时接口 ===== */

typedef struct
{
    void (*pf_os_delay_ms)(const uint32_t);  /* 延时 ms */
} xxx_os_delay_interface_t;

/* ===== xxx 驱动实例 ===== */

typedef struct xxx_private_data xxx_private_data_t;

typedef struct
{
    /* 注入的外部接口 */
    xxx_bus_interface_t        *p_bus_instance;
    xxx_timebase_interface_t   *p_timebase_instance;
    xxx_os_delay_interface_t   *p_os_delay_instance;
    xxx_private_data_t         *p_private_data;

    /* 实例函数指针（实例化时挂载） */
    xxx_status_t (*pf_inst)(void *const p_xxx_instance,
                            xxx_bus_interface_t *const p_bus_instance,
                            xxx_timebase_interface_t *const p_timebase_instance,
                            xxx_os_delay_interface_t *const p_os_delay_instance);

    xxx_status_t (*pf_init)(void *const);
    xxx_status_t (*pf_deinit)(void *const);
    xxx_status_t (*pf_read)(void *const, uint8_t *, uint32_t, uint32_t);
    xxx_status_t (*pf_write)(void *const, const uint8_t *, uint32_t, uint32_t);
    /* 按需增加：pf_erase / pf_sleep / pf_wakeup / ... */
} xxx_driver_t;

/* 实例化 xxx 驱动（挂载总线/时基/延时接口 + 初始化） */
xxx_status_t xxx_inst(
    xxx_driver_t               *const p_xxx_instance,
    xxx_bus_interface_t        *const p_bus_instance,
    xxx_timebase_interface_t   *const p_timebase_instance,
    xxx_os_delay_interface_t   *const p_os_delay_instance);

/* 读 xxx 状态（按需，如器件 ID / 睡眠状态） */
xxx_status_t read_xxx_id(xxx_driver_t *const p_xxx_instance,
                         uint16_t *const p_id);

#endif /* BSP_XXX_DRIVER_H */
