#include "bsp_adapter_port_xxx.h"

#include "bsp_xxx_driver.h"
#include "bus_port.h"          /* 换成 SPI / IIC Port 头文件 */

#ifdef XXX_BARE_METAL
/* 裸机版：无需 OS 头文件（HAL 经 bus_port.h 的 main.h 带入） */
#else
#include "bsp_xxx_handler.h"
#include "bsp_wrapper_xxx.h"
#include "osal.h"
#include "user_task_reso_config.h"
#endif

/*
 * xxx Adapter 实现（OS / 裸机二选一，用 XXX_BARE_METAL 切换）
 *
 * OS 版：把 xxx_drv_t 函数表接到 bsp_xxx_handler（发事件 + 注册函数表）。
 * 裸机版：直接同步调 Driver，对外留 xxx_bare_init/read/write。
 */

/* ===== 总线桥接：总线 Port -> driver 期望的逐字节接口（OS/裸机公共）===== */

static void bus_init(void *arg)
{
    (void)arg; /* 总线在总线配置表静态配置，无需额外初始化 */
}

static uint8_t bus_cs_set(void)
{
    /* 换成实际总线开始（如 core_spi_begin / i2c 起始） */
    return 0U;
}

static uint8_t bus_cs_deset(void)
{
    return 0U;
}

static uint8_t bus_write_byte(uint8_t data)
{
    (void)data;
    return 0U;
}

static uint8_t bus_read_byte(void)
{
    return 0U;
}

/* ===== 临界区 / 时基 / 延时（OS 用 osal，裸机用 HAL/CMSIS）===== */

#ifdef XXX_BARE_METAL
static void xxx_critical_enter(void) {}   /* 中断也访问总线时改 __disable_irq() */
static void xxx_critical_exit(void) {}
static uint32_t xxx_tick_count_get(void) { return HAL_GetTick(); }
static void xxx_delay_ms(uint32_t ms) { HAL_Delay(ms); }
#else
static void xxx_critical_enter(void) { (void)osal_critical_enter(); }
static void xxx_critical_exit(void) { (void)osal_critical_exit(); }
static uint32_t xxx_tick_count_get(void) { return osal_task_get_tick_count(); }
static void xxx_delay_ms(uint32_t ms) { osal_task_delay(osal_ms_to_ticks(ms)); }
#endif

/* ===== 接口实例（OS/裸机公共）===== */

static xxx_bus_interface_t g_busInterface = {
    .pf_bus_init       = bus_init,
    .pf_bus_deinit     = NULL,
    .pf_bus_cs_set     = bus_cs_set,
    .pf_bus_cs_deset   = bus_cs_deset,
    .pf_bus_write_byte = bus_write_byte,
    .pf_bus_read_byte  = bus_read_byte,
    .pf_critical_enter = xxx_critical_enter,
    .pf_critical_exit  = xxx_critical_exit,
};

static xxx_timebase_interface_t g_timebaseInterface = {
    .pf_tick_count_get = xxx_tick_count_get,
};

static xxx_os_delay_interface_t g_osDelayInterface = {
    .pf_os_delay_ms = xxx_delay_ms,
};

#ifdef XXX_BARE_METAL

/* ===== 裸机版：同步调 Driver ===== */

static xxx_driver_t g_xxxDriver;

int xxx_bare_init(void)
{
    return (xxx_inst(&g_xxxDriver, &g_busInterface,
                     &g_timebaseInterface, &g_osDelayInterface) == XXX_OK)
               ? 0 : -1;
}

int xxx_bare_read(uint32_t addr, uint8_t *buf, uint32_t size)
{
    return (g_xxxDriver.pf_read(&g_xxxDriver, buf, addr, size) == XXX_OK)
               ? 0 : -1;
}

int xxx_bare_write(uint32_t addr, const uint8_t *buf, uint32_t size)
{
    return (g_xxxDriver.pf_write(&g_xxxDriver, buf, addr, size) == XXX_OK)
               ? 0 : -1;
}

#else

/* ===== OS 版：发事件 + 注册函数表 ===== */

/* OS 队列桥接：学习库 OSAL -> handler 期望的函数指针签名 */
static xxx_handler_status_t xxx_os_queue_create(uint32_t item_num,
                                                uint32_t item_size,
                                                void **queue_handle)
{
    return (osal_queue_create((size_t)item_num, (size_t)item_size,
                              (osal_queue_handle_t *)queue_handle) == OSAL_SUCCESS)
               ? XXX_HANDLER_OK : XXX_HANDLER_ERROR_RESOURCE;
}

static xxx_handler_status_t xxx_os_queue_send(void *queue_handle,
                                              void *item,
                                              uint32_t timeout)
{
    return (osal_queue_send((osal_queue_handle_t)queue_handle, item,
                            (osal_tick_t)timeout) == OSAL_SUCCESS)
               ? XXX_HANDLER_OK : XXX_HANDLER_ERROR_RESOURCE;
}

static xxx_handler_status_t xxx_os_queue_receive(void *queue_handle,
                                                 void *msg,
                                                 uint32_t timeout)
{
    return (osal_queue_receive((osal_queue_handle_t)queue_handle, msg,
                               (osal_tick_t)timeout) == OSAL_SUCCESS)
               ? XXX_HANDLER_OK : XXX_HANDLER_ERROR_RESOURCE;
}

static xxx_handler_os_interface_t g_handlerOsInterface = {
    .pf_os_delay        = xxx_delay_ms,
    .pf_os_queue_create = xxx_os_queue_create,
    .pf_os_queue_receive = xxx_os_queue_receive,
    .pf_os_queue_send   = xxx_os_queue_send,
};

/* xxx handler 任务的输入参数（目标工程资源表把此变量作为任务 argument） */
xxx_handler_all_input_arg_t xxx_handler_all_input_arg = {
    .p_bus_instance      = &g_busInterface,
    .p_timebase_instance = &g_timebaseInterface,
    .p_os_instance       = &g_handlerOsInterface,
    .p_os_delay_instance = &g_osDelayInterface,
};

/* 完成回调：give 信号量（供上层 service 等待完成） */
static void read_finish_callback(void *p_buff)
{
    (void)p_buff;
    if (user_sema_get(USER_SEMA_IDX_XXX) != NULL)
    {
        (void)osal_sema_give(user_sema_get(USER_SEMA_IDX_XXX));
    }
}

static void write_finish_callback(void *p_buff)
{
    (void)p_buff;
    if (user_sema_get(USER_SEMA_IDX_XXX) != NULL)
    {
        (void)osal_sema_give(user_sema_get(USER_SEMA_IDX_XXX));
    }
}

/* xxx_drv_t 函数表：把同步接口转成发事件 */
static void xxx_drv_init(xxx_drv_t *dev)
{
    (void)dev;
}

static void xxx_drv_deinit(xxx_drv_t *dev)
{
    (void)dev;
}

static void xxx_drv_read(xxx_drv_t *dev, uint32_t addr,
                         uint32_t size, uint8_t *p_buf)
{
    xxx_handler_event_t readEvent = {
        .addr = addr,
        .size = size,
        .p_data = p_buf,
        .type = XXX_HANDLER_EVENT_READ,
        .pf_callback = read_finish_callback,
    };
    (void)dev;
    (void)xxx_handler_event_put(&readEvent);
}

static void xxx_drv_write(xxx_drv_t *dev, uint32_t addr,
                          uint32_t size, uint8_t *p_buf)
{
    xxx_handler_event_t writeEvent = {
        .addr = addr,
        .size = size,
        .p_data = p_buf,
        .type = XXX_HANDLER_EVENT_WRITE,
        .pf_callback = write_finish_callback,
    };
    (void)dev;
    (void)xxx_handler_event_put(&writeEvent);
}

void bsp_adapter_xxx_register(void)
{
    xxx_drv_t xxxDrv = {
        .idx = 0U,
        .dev_id = 0U,
        .drv_init = xxx_drv_init,
        .drv_deinit = xxx_drv_deinit,
        .drv_read = xxx_drv_read,
        .drv_write = xxx_drv_write,
    };

    (void)bsp_wrapper_xxx_reg(0U, &xxxDrv);
}

#endif /* XXX_BARE_METAL */
