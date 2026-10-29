/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_adapter_port_flash.c
 *
 * @par dependencies
 * - bsp_adapter_port_flash.h
 * - bsp_flash_handler.h
 * - bsp_wrapper_flash.h
 * - osal.h
 *
 * @author YA
 *
 * @brief 外部 Flash 平台适配层实现。
 *
 * Processing flow:
 *
 * 平台接口 -> Handler 事件 -> W25Q64 Driver -> Wrapper -> Service。
 *
 * @version V1.0 2026-10-17
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/
#include "bsp_adapter_port_flash.h"

#include "bsp_flash_handler.h"
#include "bsp_wrapper_flash.h"
#include "osal.h"
#include "spi_port.h"
#include "user_task_reso_config.h"

#include <stddef.h>

/* 前置声明：挂载函数在最上面，需要先声明下面这些函数。 */
static uint8_t spi_cs_set(void);
static uint8_t spi_cs_deset(void);
static uint8_t spi_write_byte(uint8_t data);
static uint8_t spi_read_byte(void);
static void flash_critical_enter(void);
static void flash_critical_exit(void);
static uint32_t flash_tick_count_get(void);
static void flash_delay_ms(uint32_t ms);
static flash_handler_status_t queue_create(uint32_t depth, uint32_t size,
                                           void **handle);
static flash_handler_status_t queue_send(void *handle, void *item,
                                         uint32_t timeout);
static flash_handler_status_t queue_receive(void *handle, void *item,
                                            uint32_t timeout);
static void spi_init(void *arg);
static void read_finish_callback(void *data);
static void write_finish_callback(void *data);
static void externflash_drv_init(externflash_drv_t *dev);
static void externflash_drv_deinit(externflash_drv_t *dev);
static void externflash_drv_read(externflash_drv_t *dev, uint32_t addr,
                                 uint32_t size, uint8_t *data);
static void externflash_drv_write(externflash_drv_t *dev, uint32_t addr,
                                  uint32_t size, uint8_t *data);
static void externflash_drv_write_noerase(externflash_drv_t *dev,
                                          uint32_t addr, uint32_t size,
                                          uint8_t *data);
static void externflash_drv_erasechip(externflash_drv_t *dev);
static void externflash_drv_erasesector(externflash_drv_t *dev,
                                        uint32_t addr);
static void externflash_drv_wakeup(externflash_drv_t *dev);
static void externflash_drv_sleep(externflash_drv_t *dev);
static void externflash_drv_test(externflash_drv_t *dev);

static spi_w25q64_interface_t s_spi = {
    .pf_spi_init = spi_init,                   /* SPI init */
    .pf_spi_deinit = NULL,                     /* SPI deinit */
    .pf_spi_cs_set = spi_cs_set,               /* Assert chip select */
    .pf_spi_cs_deset = spi_cs_deset,           /* Deassert chip select */
    .pf_spi_write_byte = spi_write_byte,       /* Write one byte */
    .pf_spi_read_byte = spi_read_byte,         /* Read one byte */
    .pf_critical_enter = flash_critical_enter, /* Enter critical section */
    .pf_critical_exit = flash_critical_exit    /* Exit critical section */
};

static w25qxx_timebase_interface_t s_timebase = {
    .pf_tick_count_get = flash_tick_count_get /* 获取系统时基 */
};

static w25qxx_os_delay_interface_t s_delay = {
    .pf_os_delay_ms = flash_delay_ms /* 毫秒延时 */
};

static flash_handler_os_interface_t s_os = {
    .pf_os_delay = flash_delay_ms,             /* OS 延时 */
    .pf_os_queue_create = queue_create,       /* 创建消息队列 */
    .pf_os_queue_send = queue_send,           /* 发送队列消息 */
    .pf_os_queue_receive = queue_receive      /* 接收队列消息 */
};

//flash任务需要的资源
flash_handler_all_input_arg_t flash_handler_all_input_arg = {
    .p_spi_w25q64_instance = &s_spi,           /* 注入 SPI 接口 */
    .p_timebase_instance = &s_timebase,       /* 注入时基接口 */
    .p_os_instance = &s_os,                   /* 注入 OS 接口 */
    .p_os_delay_instance = &s_delay            /* 注入延时接口 */
};

/**
 * @brief 注册外部 Flash Adapter。
 *
 * @return void : 无返回值。
 */
void bsp_adapter_flash_register(void)
{
    externflash_drv_t externflashDrv = {
        .idx = 0U,
        .dev_id = 0U,
        .user_data = NULL,
        .externflash_drv_init = externflash_drv_init,
        .externflash_drv_deinit = externflash_drv_deinit,
        .externflash_drv_read = externflash_drv_read,
        .externflash_drv_write = externflash_drv_write,
        .externflash_drv_write_noerase = externflash_drv_write_noerase,
        .externflash_drv_erasechip = externflash_drv_erasechip,
        .externflash_drv_erasesector = externflash_drv_erasesector,
        .externflash_drv_wakeup = externflash_drv_wakeup,
        .externflash_drv_sleep = externflash_drv_sleep,
        .externflash_drv_test = externflash_drv_test
    };

    (void)bsp_wrapper_flash_reg(0U, &externflashDrv);
}

/* 目标工程改这两个宏即可：SPI 逻辑槽位号和单字节超时。 */
#define FLASH_SPI_PORT       ((core_spi_port_id_t)0U)
#define FLASH_SPI_TIMEOUT_MS (1000U)

/**
 * @brief 占用 SPI 总线并拉低片选。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
static uint8_t spi_cs_set(void)
{
    return (CORE_SPI_OK == core_spi_begin(FLASH_SPI_PORT)) ? 0U : 1U;
}

/**
 * @brief 释放片选并解锁 SPI 总线。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
static uint8_t spi_cs_deset(void)
{
    return (CORE_SPI_OK == core_spi_end(FLASH_SPI_PORT)) ? 0U : 1U;
}

/**
 * @brief 发送一个 SPI 字节。
 * @param[in] data : 待发送字节。
 * @return uint8_t : 0 表示成功，1 表示失败。
 */
static uint8_t spi_write_byte(uint8_t data)
{
    return (CORE_SPI_OK ==
            core_spi_write(FLASH_SPI_PORT, &data, 1U, FLASH_SPI_TIMEOUT_MS))
               ? 0U
               : 1U;
}

/**
 * @brief 接收一个 SPI 字节。
 * @return uint8_t : 收到的字节；总线异常时返回 0xFF。
 */
static uint8_t spi_read_byte(void)
{
    uint8_t data = 0xFFU;

    (void)core_spi_read(FLASH_SPI_PORT, &data, 1U, FLASH_SPI_TIMEOUT_MS);
    return data;
}

/** @brief 进入 Flash SPI 临界区。 */
static void flash_critical_enter(void)
{
    (void)osal_critical_enter();
}

/** @brief 退出 Flash SPI 临界区。 */
static void flash_critical_exit(void)
{
    (void)osal_critical_exit();
}

/** @brief 获取当前 OSAL tick。 */
static uint32_t flash_tick_count_get(void)
{
    return osal_task_get_tick_count();
}

/** @brief 延时指定毫秒数。 */
static void flash_delay_ms(uint32_t ms)
{
    osal_task_delay(osal_ms_to_ticks(ms));
}

static flash_handler_status_t queue_create(uint32_t depth, uint32_t size,
                                           void **handle)
{
    if (NULL == handle)
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }

    return (OSAL_SUCCESS ==
            osal_queue_create((size_t)depth, (size_t)size,
                              (osal_queue_handle_t *)handle))
               ? FLASH_HANDLER_OK
               : FLASH_HANDLER_ERROR_RESOURCE;
}

static flash_handler_status_t queue_send(void *handle, void *item,
                                         uint32_t timeout)
{
    if ((NULL == handle) || (NULL == item))
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }

    return (OSAL_SUCCESS ==
            osal_queue_send((osal_queue_handle_t)handle, item,
                            (osal_tick_t)timeout))
               ? FLASH_HANDLER_OK
               : FLASH_HANDLER_ERROR_RESOURCE;
}

static flash_handler_status_t queue_receive(void *handle, void *item,
                                            uint32_t timeout)
{
    if ((NULL == handle) || (NULL == item))
    {
        return FLASH_HANDLER_ERROR_PARAMETER;
    }

    return (OSAL_SUCCESS ==
            osal_queue_receive((osal_queue_handle_t)handle, item,
                               (osal_tick_t)timeout))
               ? FLASH_HANDLER_OK
               : FLASH_HANDLER_ERROR_RESOURCE;
}

//spi底层函数挂载：SPI 时序由 spi_port 的静态配置表提供，这里无需初始化
static void spi_init(void *arg)
{
    (void)arg;
}

static void read_finish_callback(void *data)
{
    osal_sema_handle_t sema;

    (void)data;
    sema = user_sema_get(USER_SEMA_IDX_EXT_FLASH);
    if (NULL != sema)
    {
        (void)osal_sema_give(sema); /* 释放信号量 */
    }
}

static void write_finish_callback(void *data)
{
    osal_sema_handle_t sema;

    (void)data;
    sema = user_sema_get(USER_SEMA_IDX_EXT_FLASH);
    if (NULL != sema)
    {
        (void)osal_sema_give(sema); /* 释放信号量 */
    }
}

static void externflash_drv_init(externflash_drv_t *dev)
{
    (void)dev;
}

static void externflash_drv_deinit(externflash_drv_t *dev)
{
    (void)dev;
}

/**
 * @brief 投递异步 Flash 读请求。
 * @param[in] dev  : Flash 驱动实例。
 * @param[in] addr : 读取起始地址。
 * @param[in] size : 读取字节数。
 * @param[out] data : 读取缓冲区。
 */
static void externflash_drv_read(externflash_drv_t *dev, uint32_t addr,
                                 uint32_t size, uint8_t *data)
{
    flash_handler_event_t event = {
        .addr = addr,
        .size = size,
        .p_data = data,
        .type = FLASH_HANDLER_EVENT_READ,
        .pf_callback = read_finish_callback
    };

    (void)dev;
    (void)flash_handler_event_put(&event);//发送队列,通知handler_task读取
}

static void externflash_drv_write(externflash_drv_t *dev, uint32_t addr,
                                  uint32_t size, uint8_t *data)
{
    flash_handler_event_t event = {
        .addr = addr,
        .size = size,
        .p_data = data,
        .type = FLASH_HANDLER_EVENT_WRITE,
        .pf_callback = write_finish_callback
    };

    (void)dev;
    (void)flash_handler_event_put(&event);//发送队列,通知handler_task写入
}

//不擦除直接写(一般是前面擦除过了)
static void externflash_drv_write_noerase(externflash_drv_t *dev,
                                          uint32_t addr, uint32_t size,
                                          uint8_t *data)
{
    flash_handler_event_t event = {
        .addr = addr,
        .size = size,
        .p_data = data,
        .type = FLASH_HANDLER_EVENT_WRITE_NOERASE,
        .pf_callback = write_finish_callback
    };

    (void)dev;
    //发送队列,通知handler_task擦除整个芯片
    (void)flash_handler_event_put(&event);
}

static void externflash_drv_erasechip(externflash_drv_t *dev)
{
    flash_handler_event_t event = {
        .addr = 0U,
        .size = 0U,
        .p_data = NULL,
        .type = FLASH_HANDLER_EVENT_ERASE_CHIP,
        .pf_callback = write_finish_callback
    };

    (void)dev;
    (void)flash_handler_event_put(&event);//发送队列,通知handler_task擦除
}


static void externflash_drv_erasesector(externflash_drv_t *dev, uint32_t addr)
{
    flash_handler_event_t event = {
        .addr = addr,
        .size = 0U,
        .p_data = NULL,
        .type = FLASH_HANDLER_EVENT_ERASE_SECTOR,
        .pf_callback = write_finish_callback
    };

    (void)dev;
    (void)flash_handler_event_put(&event);//发送队列,通知handler_task擦除一个扇区
}

static void externflash_drv_wakeup(externflash_drv_t *dev)
{
    (void)dev;
}

static void externflash_drv_sleep(externflash_drv_t *dev)
{
    (void)dev;
}

static void externflash_drv_test(externflash_drv_t *dev)
{
    (void)dev;
}
