#include "bsp_adapter_port_flash.h"

#include "bsp_w25q64_driver.h"
#include "spi_port.h"          /* core_spi_*；HAL_GetTick/HAL_Delay 经 main.h 带入 */

#ifdef FLASH_BARE_METAL
/* 裸机版：无需 OS 头文件（HAL 经 spi_port.h 的 main.h 带入） */
#else
#include "bsp_flash_handler.h"
#include "bsp_wrapper_flash.h"
#include "osal.h"
#include "user_task_reso_config.h"
#endif

/*
 * 外部 Flash Adapter 实现（OS / 裸机二选一，用 FLASH_BARE_METAL 切换）
 *
 * OS 版：把 externflash_drv_t 函数表接到 bsp_flash_handler（发事件 + 注册函数表）。
 * 裸机版：直接同步调 Driver，对外留 flash_bare_init/read/write/erase_all。
 */

/* ===== 桥接函数：core_spi -> driver 期望的逐字节 SPI 接口（OS/裸机公共）===== */

static void spi_init(void *arg)
{
    (void)arg; /* core_spi 在 spi_port_config.h 静态表配置，无需额外初始化 */
}

static uint8_t spi_cs_set(void)
{
    return (core_spi_begin((core_spi_port_id_t)FLASH_SPI_PORT) == CORE_SPI_OK)
               ? 0U : 1U;
}

static uint8_t spi_cs_deset(void)
{
    return (core_spi_end((core_spi_port_id_t)FLASH_SPI_PORT) == CORE_SPI_OK)
               ? 0U : 1U;
}

static uint8_t spi_write_byte(uint8_t data)
{
    return (core_spi_write((core_spi_port_id_t)FLASH_SPI_PORT, &data, 1U,
                           FLASH_SPI_TIMEOUT_MS) == CORE_SPI_OK) ? 0U : 1U;
}

static uint8_t spi_read_byte(void)
{
    uint8_t data = 0U;
    (void)core_spi_read((core_spi_port_id_t)FLASH_SPI_PORT, &data, 1U,
                        FLASH_SPI_TIMEOUT_MS);
    return data;
}

/* ===== 临界区 / 时基 / 延时（OS 用 osal，裸机用 HAL/CMSIS）===== */

#ifdef FLASH_BARE_METAL
static void flash_critical_enter(void) {}   /* 中断也访问 SPI 时改 __disable_irq() */
static void flash_critical_exit(void) {}
static uint32_t flash_tick_count_get(void) { return HAL_GetTick(); }
static void flash_delay_ms(uint32_t ms) { HAL_Delay(ms); }
#else
static void flash_critical_enter(void) { (void)osal_critical_enter(); }
static void flash_critical_exit(void) { (void)osal_critical_exit(); }
static uint32_t flash_tick_count_get(void) { return osal_task_get_tick_count(); }
static void flash_delay_ms(uint32_t ms) { osal_task_delay(osal_ms_to_ticks(ms)); }
#endif

/* ===== 接口实例（OS/裸机公共）===== */

static spi_w25q64_interface_t g_spiW25q64Interface = {
    .pf_spi_init       = spi_init,
    .pf_spi_deinit     = NULL,
    .pf_spi_cs_set     = spi_cs_set,
    .pf_spi_cs_deset   = spi_cs_deset,
    .pf_spi_write_byte = spi_write_byte,
    .pf_spi_read_byte  = spi_read_byte,
    .pf_critical_enter = flash_critical_enter,
    .pf_critical_exit  = flash_critical_exit,
};

static w25qxx_timebase_interface_t g_timebaseInterface = {
    .pf_tick_count_get = flash_tick_count_get,
};

static w25qxx_os_delay_interface_t g_osDelayInterface = {
    .pf_os_delay_ms = flash_delay_ms,
};

#ifdef FLASH_BARE_METAL

/* ===== 裸机版：同步调 Driver ===== */

static bsp_w25q64_driver_t g_flashDriver;

int flash_bare_init(void)
{
    return (w25q64_inst(&g_flashDriver, &g_spiW25q64Interface,
                        &g_timebaseInterface, &g_osDelayInterface) == W25Q64_OK)
               ? 0 : -1;
}

int flash_bare_read(uint32_t addr, uint8_t *buf, uint32_t size)
{
    return (g_flashDriver.pf_read(&g_flashDriver, buf, addr, size) == W25Q64_OK)
               ? 0 : -1;
}

int flash_bare_write(uint32_t addr, const uint8_t *buf, uint32_t size)
{
    return (g_flashDriver.pf_write_erase(&g_flashDriver, buf, addr, size) == W25Q64_OK)
               ? 0 : -1;
}

int flash_bare_erase_all(void)
{
    return (g_flashDriver.pf_erase_chip(&g_flashDriver) == W25Q64_OK) ? 0 : -1;
}

#else

/* ===== OS 版：发事件 + 注册函数表 ===== */

/* 桥接函数：学习库 OSAL -> handler 期望的函数指针签名 */
static flash_handler_status_t flash_os_queue_create(uint32_t item_num,
                                                     uint32_t item_size,
                                                     void **queue_handle)
{
    return (osal_queue_create((size_t)item_num, (size_t)item_size,
                              (osal_queue_handle_t *)queue_handle) == OSAL_SUCCESS)
               ? FLASH_HANDLER_OK : FLASH_HANDLER_ERROR_RESOURCE;
}

static flash_handler_status_t flash_os_queue_send(void *queue_handle,
                                                   void *item,
                                                   uint32_t timeout)
{
    return (osal_queue_send((osal_queue_handle_t)queue_handle, item,
                            (osal_tick_t)timeout) == OSAL_SUCCESS)
               ? FLASH_HANDLER_OK : FLASH_HANDLER_ERROR_RESOURCE;
}

static flash_handler_status_t flash_os_queue_receive(void *queue_handle,
                                                      void *msg,
                                                      uint32_t timeout)
{
    return (osal_queue_receive((osal_queue_handle_t)queue_handle, msg,
                               (osal_tick_t)timeout) == OSAL_SUCCESS)
               ? FLASH_HANDLER_OK : FLASH_HANDLER_ERROR_RESOURCE;
}

static flash_handler_os_interface_t g_flashHandlerOsInterface = {
    .pf_os_delay        = flash_delay_ms,
    .pf_os_queue_create = flash_os_queue_create,
    .pf_os_queue_receive = flash_os_queue_receive,
    .pf_os_queue_send   = flash_os_queue_send,
};

/* flash handler 任务的输入参数（目标工程资源表把此变量作为任务 argument） */
flash_handler_all_input_arg_t flash_handler_all_input_arg = {
    .p_spi_w25q64_instance = &g_spiW25q64Interface,
    .p_timebase_instance   = &g_timebaseInterface,
    .p_os_instance         = &g_flashHandlerOsInterface,
    .p_os_delay_instance   = &g_osDelayInterface,
};

/* ===== 完成回调：give 信号量（供上层 service 等待完成） ===== */

static void read_finish_callback(void *p_buff)
{
    (void)p_buff;
    if (user_sema_get(USER_SEMA_IDX_EXT_FLASH) != NULL)
    {
        (void)osal_sema_give(user_sema_get(USER_SEMA_IDX_EXT_FLASH));
    }
}

static void write_finish_callback(void *p_buff)
{
    (void)p_buff;
    if (user_sema_get(USER_SEMA_IDX_EXT_FLASH) != NULL)
    {
        (void)osal_sema_give(user_sema_get(USER_SEMA_IDX_EXT_FLASH));
    }
}

/* ===== externflash_drv_t 函数表：把同步接口转成发事件 ===== */

static void externflash_drv_init(externflash_drv_t *dev)
{
    (void)dev;
}

static void externflash_drv_deinit(externflash_drv_t *dev)
{
    (void)dev;
}

static void externflash_drv_read(externflash_drv_t *dev, uint32_t addr,
                                 uint32_t size, uint8_t *p_buf)
{
    flash_handler_event_t readEvent = {
        .addr = addr,
        .size = size,
        .p_data = p_buf,
        .type = FLASH_HANDLER_EVENT_READ,
        .pf_callback = read_finish_callback,
    };
    (void)dev;
    (void)flash_handler_event_put(&readEvent);
}

static void externflash_drv_write(externflash_drv_t *dev, uint32_t addr,
                                  uint32_t size, uint8_t *p_buf)
{
    flash_handler_event_t writeEvent = {
        .addr = addr,
        .size = size,
        .p_data = p_buf,
        .type = FLASH_HANDLER_EVENT_WRITE,
        .pf_callback = write_finish_callback,
    };
    (void)dev;
    (void)flash_handler_event_put(&writeEvent);
}

static void externflash_drv_write_noerase(externflash_drv_t *dev,
                                          uint32_t addr, uint32_t size,
                                          uint8_t *p_buf)
{
    flash_handler_event_t writeEvent = {
        .addr = addr,
        .size = size,
        .p_data = p_buf,
        .type = FLASH_HANDLER_EVENT_WRITE_NOERASE,
        .pf_callback = write_finish_callback,
    };
    (void)dev;
    (void)flash_handler_event_put(&writeEvent);
}

static void externflash_drv_erasechip(externflash_drv_t *dev)
{
    (void)dev;
}

static void externflash_drv_erasesector(externflash_drv_t *dev, uint32_t addr)
{
    flash_handler_event_t eraseEvent = {
        .addr = addr,
        .size = 0U,
        .p_data = NULL,
        .type = FLASH_HANDLER_EVENT_ERASE_SECTOR,
        .pf_callback = write_finish_callback,
    };
    (void)dev;
    (void)flash_handler_event_put(&eraseEvent);
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

/* ===== 注册入口 ===== */

void bsp_adapter_flash_register(void)
{
    externflash_drv_t externflashDrv = {
        .idx = 0U,
        .dev_id = 0U,
        .externflash_drv_init = externflash_drv_init,
        .externflash_drv_deinit = externflash_drv_deinit,
        .externflash_drv_read = externflash_drv_read,
        .externflash_drv_write = externflash_drv_write,
        .externflash_drv_write_noerase = externflash_drv_write_noerase,
        .externflash_drv_erasechip = externflash_drv_erasechip,
        .externflash_drv_erasesector = externflash_drv_erasesector,
        .externflash_drv_wakeup = externflash_drv_wakeup,
        .externflash_drv_sleep = externflash_drv_sleep,
        .externflash_drv_test = externflash_drv_test,
    };

    (void)bsp_wrapper_flash_reg(0U, &externflashDrv);
}

#endif /* FLASH_BARE_METAL */
