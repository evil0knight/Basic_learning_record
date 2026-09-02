#ifndef BSP_W25Q64_DRIVER_H
#define BSP_W25Q64_DRIVER_H

#include <stdint.h>
#include "bsp_w25q64_reg.h"

/*
 * W25Q64 驱动层（照手表 bsp_w25q64_driver 搬运）
 * 依赖全部通过函数指针接口注入，不直接依赖 OS / SPI 类型：
 *   - spi_w25q64_interface_t：SPI 逐字节接口（由 Adapter 用 core_spi 实现）
 *   - w25qxx_timebase_interface_t：时基（pf_tick_count_get）
 *   - w25qxx_os_delay_interface_t：延时（pf_os_delay_ms）
 */

/* 学习库恒使用 OS 抽象 + 硬件 SPI */
#define OS_SUPPORT
#define USE_HARDWARE_SPI

/* W25Q64 状态返回码 */
typedef enum
{
    W25Q64_OK = 0,
    W25Q64_ERROR,
    W25Q64_ERROR_TIMEOUT,
    W25Q64_ERROR_RESOURCE,
    W25Q64_ERROR_PARAMETER,
    W25Q64_ERROR_NOMEM,
    W25Q64_BUSY,
    W25Q64_SLEEP,
    W25Q64_WAKEUP,
    W25Q64_ERROR_ISR,
    W25Q64_ERROR_UNKNOWN,
    W25Q64_RESERVED = 0x1FFFFFFF,
} w25q64_status_t;

/* ===== SPI 逐字节接口（来自 core 层 SPI Port，由 Adapter 适配）===== */

typedef struct
{
    void    (*pf_spi_init)(void *);      /* SPI 初始化 */
    void    (*pf_spi_deinit)(void *);    /* SPI 去初始化 */
    uint8_t (*pf_spi_cs_set)(void);      /* 片选拉低 */
    uint8_t (*pf_spi_cs_deset)(void);    /* 片选拉高 */
    uint8_t (*pf_spi_write_byte)(uint8_t); /* 写一个字节 */
    uint8_t (*pf_spi_read_byte)(void);     /* 读一个字节 */

    /* OS 临界区（保证 CS 期间数据完整） */
    void    (*pf_critical_enter)(void);
    void    (*pf_critical_exit)(void);
} spi_w25q64_interface_t;

/* ===== 时基接口 ===== */

typedef struct
{
    uint32_t (*pf_tick_count_get)(void);  /* 获取 tick */
} w25qxx_timebase_interface_t;

/* ===== OS 延时接口 ===== */

typedef struct
{
    void (*pf_os_delay_ms)(const uint32_t);  /* 延时 ms */
} w25qxx_os_delay_interface_t;

/* ===== W25Q64 驱动实例 ===== */

typedef struct w25q64_private_data w25q64_private_data_t;

typedef struct
{
    /* 注入的外部接口 */
    spi_w25q64_interface_t       *p_spi_w25q64_instance;
    w25qxx_timebase_interface_t  *p_timebase_instance;
    w25qxx_os_delay_interface_t  *p_os_delay_instance;
    w25q64_private_data_t        *p_private_data;

    /* 实例函数指针（实例化时挂载） */
    w25q64_status_t (*pf_inst)(void *const p_w25q64_instance,
                               spi_w25q64_interface_t *const p_spi_w25q64_instance,
                               w25qxx_timebase_interface_t *const p_timebase_instance,
                               w25qxx_os_delay_interface_t *const p_os_delay_instance);

    w25q64_status_t (*pf_init)(void *const);
    w25q64_status_t (*pf_deinit)(void *const);
    w25q64_status_t (*pf_read_id)(void *const, uint16_t *const);
    w25q64_status_t (*pf_read)(void *const, uint8_t *, uint32_t, uint32_t);
    w25q64_status_t (*pf_write_erase)(void *const, const uint8_t *, uint32_t, uint32_t);
    w25q64_status_t (*pf_write_no_erase)(void *const, const uint8_t *, uint32_t, uint32_t);
    w25q64_status_t (*pf_erase_chip)(void *const);
    w25q64_status_t (*pf_sleep)(void *const);
    w25q64_status_t (*pf_wakeup)(void *const);
} bsp_w25q64_driver_t;

/* 实例化 W25Q64 驱动（挂载 SPI/时基/延时接口 + 初始化） */
w25q64_status_t w25q64_inst(
    bsp_w25q64_driver_t      *const p_w25q64_instance,
    spi_w25q64_interface_t   *const p_spi_w25q64_instance,
    w25qxx_timebase_interface_t *const p_timebase_instance,
    w25qxx_os_delay_interface_t *const p_os_delay_instance);

/* 读 W25Q64 ID */
w25q64_status_t read_w25qxx_id(bsp_w25q64_driver_t *const p_w25q64_instance,
                               uint16_t *const p_id);

/* 读 W25Q64 睡眠状态 */
w25q64_status_t read_w25qxx_isSleep(bsp_w25q64_driver_t *const p_w25q64_instance);

#endif /* BSP_W25Q64_DRIVER_H */
