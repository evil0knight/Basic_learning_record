#ifndef BSP_WRAPPER_XXX_H
#define BSP_WRAPPER_XXX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * xxx Wrapper（函数表注册模式）
 *
 * 本层只做一件事：保存上层注册的 xxx_drv_t 函数表，并把 read/write 等
 * 调用转发到注册的驱动。Wrapper 不包含任何器件/OS 类型，通过 bsp_wrapper_xxx_reg
 * 注册驱动后即可使用。
 *
 * 使用方式：
 *   1. 在 Adapter 里实现 xxx_drv_t 函数表；
 *   2. 初始化时调用 bsp_wrapper_xxx_reg(index, &drv) 注册；
 *   3. 上层只调用 bsp_wrapper_xxx_read/write 等接口。
 */

/* xxx 驱动函数表：成员按外设增删 */
typedef struct _xxx_drv_t
{
    uint32_t idx;      /* 实例索引 */
    uint32_t dev_id;   /* 器件 id */
    void *user_data;   /* 用户数据 */

    void (*drv_init)(struct _xxx_drv_t *dev);
    void (*drv_deinit)(struct _xxx_drv_t *dev);
    void (*drv_read)(struct _xxx_drv_t *dev,
                     uint32_t addr, uint32_t size, uint8_t *p_buf);
    void (*drv_write)(struct _xxx_drv_t *dev,
                      uint32_t addr, uint32_t size, uint8_t *p_buf);
    /* 按需增加：erase / wakeup / sleep / test / ... */
} xxx_drv_t;

/* 注册一个 xxx 驱动函数表 */
bool bsp_wrapper_xxx_reg(uint32_t index, xxx_drv_t *dev);

void bsp_wrapper_xxx_init(void);
void bsp_wrapper_xxx_deinit(void);
void bsp_wrapper_xxx_read(uint32_t addr, uint32_t size, uint8_t *p_buf);
void bsp_wrapper_xxx_write(uint32_t addr, uint32_t size, uint8_t *p_buf);

#endif /* BSP_WRAPPER_XXX_H */
