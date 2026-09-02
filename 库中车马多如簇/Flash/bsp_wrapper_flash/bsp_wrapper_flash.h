#ifndef BSP_WRAPPER_FLASH_H
#define BSP_WRAPPER_FLASH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * 外部 Flash Wrapper（函数表注册模式）
 *
 * 本层只做一件事：保存上层注册的 externflash_drv_t 函数表，并把 read/write 等
 * 调用转发到注册的驱动。Wrapper 不包含任何器件/OS 类型，通过 bsp_wrapper_flash_reg
 * 注册驱动后即可使用。
 *
 * 使用方式：
 *   1. 在 Adapter 里实现 externflash_drv_t 函数表；
 *   2. 初始化时调用 bsp_wrapper_flash_reg(index, &drv) 注册；
 *   3. 上层只调用 bsp_wrapper_flash_read/write 等接口。
 */

/* 外部 Flash 驱动函数表 */
typedef struct _externflash_drv_t
{
    uint32_t idx;      /* 实例索引 */
    uint32_t dev_id;   /* 器件 id */
    void *user_data;   /* 用户数据 */

    void (*externflash_drv_init)(struct _externflash_drv_t *dev);
    void (*externflash_drv_deinit)(struct _externflash_drv_t *dev);
    void (*externflash_drv_read)(struct _externflash_drv_t *dev,
                                 uint32_t addr, uint32_t size, uint8_t *p_buf);
    void (*externflash_drv_write)(struct _externflash_drv_t *dev,
                                  uint32_t addr, uint32_t size, uint8_t *p_buf);
    void (*externflash_drv_write_noerase)(struct _externflash_drv_t *dev,
                                          uint32_t addr, uint32_t size, uint8_t *p_buf);
    void (*externflash_drv_erasechip)(struct _externflash_drv_t *dev);
    void (*externflash_drv_erasesector)(struct _externflash_drv_t *dev, uint32_t addr);
    void (*externflash_drv_wakeup)(struct _externflash_drv_t *dev);
    void (*externflash_drv_sleep)(struct _externflash_drv_t *dev);
    void (*externflash_drv_test)(struct _externflash_drv_t *dev);
} externflash_drv_t;

/* 注册一个外部 Flash 驱动函数表 */
bool bsp_wrapper_flash_reg(uint32_t index, externflash_drv_t *dev);

void bsp_wrapper_flash_init(void);
void bsp_wrapper_flash_deinit(void);
void bsp_wrapper_flash_read(uint32_t addr, uint32_t size, uint8_t *p_buf);
void bsp_wrapper_flash_write(uint32_t addr, uint32_t size, uint8_t *p_buf);
void bsp_wrapper_flash_write_noerase(uint32_t addr, uint32_t size, uint8_t *p_buf);
void bsp_wrapper_flash_erasechip(void);
void bsp_wrapper_flash_erasesector(uint32_t addr);
void bsp_wrapper_flash_wakeup(void);
void bsp_wrapper_flash_sleep(void);
void bsp_wrapper_flash_test(void);

#endif /* BSP_WRAPPER_FLASH_H */
