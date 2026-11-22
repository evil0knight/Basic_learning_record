/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file i2c_port_os.c
 *
 * @par dependencies
 * - i2c_port_os.h
 * - i2c_bus.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief RTOS I2C Port 实现，锁与信号量由集中资源表注册后在本层使用。
 *
 * Processing flow:
 *
 * 注册句柄 -> 同步接口加锁 + 启动 DMA + 等信号量 + 解锁
 * -> 异步接口启动 DMA，完成回调里通知调用方。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "i2c_port_os.h"

#include "../../bus/inc/i2c_bus.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//**************************** Typedefs begin ********************************//
/* 每个逻辑总线槽位一份：资源、结果和异步回调都收在这里。 */
typedef struct
{
    osal_mutex_handle_t           mutex;                // 总线互斥量，NULL 表示不加锁
    osal_sema_handle_t            sema;                 // 异步完成信号量，NULL 表示只靠回调
    volatile en_core_i2c_status_t status;               // 本次传输结果
    volatile uint32_t             error_code;           // HAL 原始错误码
    uint8_t                       device_address_7bit;  // 本次传输设备地址
    core_i2c_done_callback_t      pf_done;              // 异步完成回调
    void                         *p_user_data;          // 异步回调上下文
} st_i2c_port_os_t;
//****************************** Typedefs end ********************************//

//************************** Static Variables begin **************************//
static st_i2c_port_os_t g_i2c_os[CORE_I2C_PORT_MAX];
//*************************** Static Variables end ***************************//

//************************** Static Functions begin **************************//
/**
 * @brief 把毫秒超时换算成 OSAL 节拍，0 表示永久等待。
 *
 * Steps:
 *  1. 0 毫秒返回 OSAL_WAIT_FOREVER。
 *  2. 其余按工程节拍换算。
 *
 * @param[in]  timeout_ms           : 超时毫秒数。
 *
 * @return osal_tick_t : 换算后的节拍数。
 */
static osal_tick_t i2c_port_os_ticks(uint32_t timeout_ms)
{
    if (0U == timeout_ms)
    {
        return OSAL_WAIT_FOREVER;
    }

    return osal_ms_to_ticks(timeout_ms);
}

/**
 * @brief 取总线互斥量，未注册时直接放行。
 *
 * Steps:
 *  1. NULL 句柄视为不加锁，直接成功。
 *  2. 其余调用 osal_mutex_take。
 *
 * @param[in]  mutex                : 总线互斥量句柄，可为 NULL。
 * @param[in]  ticks                : 等待节拍数。
 *
 * @return en_core_i2c_status_t : 取锁结果。
 */
static en_core_i2c_status_t i2c_port_os_lock(
    osal_mutex_handle_t  mutex,
    osal_tick_t          ticks)
{
    if (NULL == mutex)
    {
        return CORE_I2C_OK;
    }

    if (OSAL_SUCCESS != osal_mutex_take(mutex, ticks))
    {
        return CORE_I2C_ERR_BUSY;
    }

    return CORE_I2C_OK;
}

/**
 * @brief 释放总线互斥量，未注册时直接返回。
 *
 * Steps:
 *  1. NULL 句柄视为不加锁，直接返回。
 *  2. 其余调用 osal_mutex_give。
 *
 * @param[in]  mutex                : 总线互斥量句柄，可为 NULL。
 *
 * @return void : 无返回值。
 */
static void i2c_port_os_unlock(osal_mutex_handle_t mutex)
{
    if (NULL != mutex)
    {
        (void)osal_mutex_give(mutex);
    }
}

/**
 * @brief 传输完成公共处理，给信号量并调用异步回调。
 *
 * Steps:
 *  1. 检查槽位范围并保存结果。
 *  2. 给完成信号量。
 *  3. 有异步回调时在中断上下文回调。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  status               : 本次传输结果。
 *
 * @return void : 无返回值。
 */
static void i2c_port_os_notify(
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    en_core_i2c_status_t  status)
{
    st_i2c_port_os_t *bus;
    core_i2c_done_callback_t pf_done;
    void *p_user_data;

    if (port >= CORE_I2C_PORT_MAX)
    {
        return;
    }

    bus = &g_i2c_os[port];
    bus->status = status;

    if (NULL != g_i2c_os[port].sema)
    {
        (void)osal_sema_give(g_i2c_os[port].sema);
    }

    pf_done = bus->pf_done;
    p_user_data = bus->p_user_data;
    bus->pf_done = NULL;

    if (NULL != pf_done)
    {
        pf_done(port, device_address_7bit, status, p_user_data);
    }
}

/**
 * @brief 读完成回调，转发给公共完成处理。
 *
 * Steps:
 *  1. 忽略读缓冲区信息。
 *  2. 调用 i2c_port_os_notify。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  status               : 本次传输结果。
 * @param[in]  p_data               : 接收缓冲区。
 * @param[in]  size                 : 接收字节数。
 *
 * @return void : 无返回值。
 */
static void i2c_port_os_rx_cb(
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    en_core_i2c_status_t  status,
    uint8_t              *p_data,
    uint16_t              size)
{
    (void)p_data;
    (void)size;

    i2c_port_os_notify(port, device_address_7bit, status);
}

/**
 * @brief 写完成回调，转发给公共完成处理。
 *
 * Steps:
 *  1. 调用 i2c_port_os_notify。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  status               : 本次传输结果。
 *
 * @return void : 无返回值。
 */
static void i2c_port_os_tx_cb(
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    en_core_i2c_status_t  status)
{
    i2c_port_os_notify(port, device_address_7bit, status);
}

/**
 * @brief 错误回调，记录 HAL 原始错误码。
 *
 * Steps:
 *  1. 检查槽位范围。
 *  2. 保存 HAL 错误码，供诊断使用。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  error_code           : HAL 原始错误码。
 *
 * @return void : 无返回值。
 */
static void i2c_port_os_error_cb(
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    uint32_t              error_code)
{
    (void)device_address_7bit;

    if (port >= CORE_I2C_PORT_MAX)
    {
        return;
    }

    g_i2c_os[port].error_code = error_code;
}
//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//

//************************** 公共接口 begin **********************************//

/**
 * @brief 初始化 Bus 并注册 Port 的读、写、错误回调。
 *
 * Steps:
 *  1. 清空 Bus 槽位的 busy 与回调表。
 *  2. 清空每个槽位的资源与完成状态。
 *  3. 为每个槽位注册回调。
 *
 * @return en_core_i2c_status_t : Bus 初始化结果。
 */
en_core_i2c_status_t core_i2c_init(void)
{
    en_core_i2c_status_t status;
    core_i2c_port_id_t   port;

    status = i2c_bus_init();
    if (CORE_I2C_OK != status)
    {
        return status;
    }

    for (port = 0U; port < CORE_I2C_PORT_MAX; ++port)
    {
        g_i2c_os[port].mutex = NULL;
        g_i2c_os[port].sema = NULL;
        g_i2c_os[port].status = CORE_I2C_ERROR;
        g_i2c_os[port].error_code = 0U;
        g_i2c_os[port].device_address_7bit = 0U;
        g_i2c_os[port].pf_done = NULL;
        g_i2c_os[port].p_user_data = NULL;

        (void)i2c_bus_register_rx_callback(port, i2c_port_os_rx_cb);
        (void)i2c_bus_register_tx_callback(port, i2c_port_os_tx_cb);
        (void)i2c_bus_register_error_callback(port, i2c_port_os_error_cb);
    }

    return CORE_I2C_OK;
}

/**
 * @brief 为指定总线注册互斥量。
 *
 * Steps:
 *  1. 检查槽位范围。
 *  2. 保存句柄，NULL 表示该总线不加锁。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  mutex                : 集中创建的互斥量句柄，可为 NULL。
 *
 * @return en_core_i2c_status_t : 注册结果。
 */
en_core_i2c_status_t core_i2c_register_mutex(
    core_i2c_port_id_t   port,
    osal_mutex_handle_t  mutex)
{
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    g_i2c_os[port].mutex = mutex;
    return CORE_I2C_OK;
}

/**
 * @brief 为指定总线注册完成信号量。
 *
 * Steps:
 *  1. 检查槽位范围。
 *  2. 保存句柄，NULL 表示异步只靠回调通知。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  sema                 : 集中创建的信号量句柄，可为 NULL。
 *
 * @return en_core_i2c_status_t : 注册结果。
 */
en_core_i2c_status_t core_i2c_register_semaphore(
    core_i2c_port_id_t  port,
    osal_sema_handle_t  sema)
{
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    g_i2c_os[port].sema = sema;
    return CORE_I2C_OK;
}
//*************************** 公共接口 end ***********************************//

//************************** 死等接口 begin **********************************//

/**
 * @brief 同步读寄存器。
 *
 * Steps:
 *  1. 加锁，锁覆盖整个传输。
 *  2. 软件槽位走临界区加普通 HAL。
 *  3. 硬件槽位启动 DMA 后等信号量，超时则 abort。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[out] p_data               : 接收缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  timeout_ms           : 传递超时，0 表示永久等待。
 *
 * @return en_core_i2c_status_t : 传输结果。
 */
en_core_i2c_status_t core_i2c_mem_read(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms)
{
    en_core_i2c_status_t status;
    osal_tick_t          ticks;

    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    /* 软件槽位必须用临界区，硬件槽位用总线互斥量。 */
    if (0U != i2c_bus_is_software(port))
    {
        (void)osal_critical_enter();
        status = i2c_bus_mem_read(port, device_address_7bit,
                                  memory_address, address_size,
                                  p_data, size, timeout_ms);
        (void)osal_critical_exit();
        return status;
    }

    ticks = i2c_port_os_ticks(timeout_ms);

    status = i2c_port_os_lock(g_i2c_os[port].mutex, ticks);
    if (CORE_I2C_OK != status)
    {
        return status;
    }

    status = i2c_bus_mem_read(port, device_address_7bit,
                              memory_address, address_size,
                              p_data, size, timeout_ms);

    i2c_port_os_unlock(g_i2c_os[port].mutex);
    return status;
}

/**
 * @brief 同步写寄存器。
 *
 * Steps:
 *  1. 加锁，锁覆盖整个传输。
 *  2. 软件槽位走临界区加普通 HAL。
 *  3. 硬件槽位启动 DMA 后等信号量，超时则 abort。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[in]  p_data               : 发送缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  timeout_ms           : 传递超时，0 表示永久等待。
 *
 * @return en_core_i2c_status_t : 传输结果。
 */
en_core_i2c_status_t core_i2c_mem_write(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms)
{
    en_core_i2c_status_t status;
    osal_tick_t          ticks;

    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    /* 软件槽位必须用临界区，硬件槽位用总线互斥量。 */
    if (0U != i2c_bus_is_software(port))
    {
        (void)osal_critical_enter();
        status = i2c_bus_mem_write(port, device_address_7bit,
                                   memory_address, address_size,
                                   p_data, size, timeout_ms);
        (void)osal_critical_exit();
        return status;
    }

    ticks = i2c_port_os_ticks(timeout_ms);

    status = i2c_port_os_lock(g_i2c_os[port].mutex, ticks);
    if (CORE_I2C_OK != status)
    {
        return status;
    }

    status = i2c_bus_mem_write(port, device_address_7bit,
                               memory_address, address_size,
                               p_data, size, timeout_ms);

    i2c_port_os_unlock(g_i2c_os[port].mutex);
    return status;
}
//*************************** 死等接口 end ***********************************//

//************************** 同步接口 begin **********************************//

/**
 * @brief DMA 读寄存器，本函数内等完成信号量。
 *
 * Steps:
 *  1. 取总线锁，清掉残留信号量。
 *  2. 启动 DMA，然后在本函数内等信号量。
 *  3. 超时则 abort 并返回 TIMEOUT。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[out] p_data               : 接收缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  timeout_ms           : 传输阶段超时，0 表示不限时。
 *
 * @return en_core_i2c_status_t : 传输结果。
 */
en_core_i2c_status_t core_i2c_mem_read_sync(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms)
{
    en_core_i2c_status_t status;
    osal_tick_t          ticks;

    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    ticks = i2c_port_os_ticks(timeout_ms);

    status = i2c_port_os_lock(g_i2c_os[port].mutex, ticks);
    if (CORE_I2C_OK != status)
    {
        return status;
    }

    (void)osal_sema_take(g_i2c_os[port].sema, 0U);
    g_i2c_os[port].pf_done = NULL;
    g_i2c_os[port].status = CORE_I2C_ERROR;

    status = i2c_bus_mem_read_dma(port, device_address_7bit,
                                  memory_address, address_size,
                                  p_data, size);
    if (CORE_I2C_OK != status)
    {
        i2c_port_os_unlock(g_i2c_os[port].mutex);
        return status;
    }

    if (OSAL_SUCCESS != osal_sema_take(g_i2c_os[port].sema, ticks))
    {
        (void)i2c_bus_abort_dma(port);
        i2c_port_os_unlock(g_i2c_os[port].mutex);
        return CORE_I2C_ERR_TIMEOUT;
    }

    status = g_i2c_os[port].status;
    i2c_port_os_unlock(g_i2c_os[port].mutex);
    return status;
}

/**
 * @brief DMA 写寄存器，本函数内等完成信号量。
 *
 * Steps:
 *  1. 取总线锁，清掉残留信号量。
 *  2. 启动 DMA，然后在本函数内等信号量。
 *  3. 超时则 abort 并返回 TIMEOUT。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[in]  p_data               : 发送缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  timeout_ms           : 传输阶段超时，0 表示不限时。
 *
 * @return en_core_i2c_status_t : 传输结果。
 */
en_core_i2c_status_t core_i2c_mem_write_sync(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms)
{
    en_core_i2c_status_t status;
    osal_tick_t          ticks;

    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    ticks = i2c_port_os_ticks(timeout_ms);

    status = i2c_port_os_lock(g_i2c_os[port].mutex, ticks);
    if (CORE_I2C_OK != status)
    {
        return status;
    }

    (void)osal_sema_take(g_i2c_os[port].sema, 0U);
    g_i2c_os[port].pf_done = NULL;
    g_i2c_os[port].status = CORE_I2C_ERROR;

    status = i2c_bus_mem_write_dma(port, device_address_7bit,
                                   memory_address, address_size,
                                   p_data, size);
    if (CORE_I2C_OK != status)
    {
        i2c_port_os_unlock(g_i2c_os[port].mutex);
        return status;
    }

    if (OSAL_SUCCESS != osal_sema_take(g_i2c_os[port].sema, ticks))
    {
        (void)i2c_bus_abort_dma(port);
        i2c_port_os_unlock(g_i2c_os[port].mutex);
        return CORE_I2C_ERR_TIMEOUT;
    }

    status = g_i2c_os[port].status;
    i2c_port_os_unlock(g_i2c_os[port].mutex);
    return status;
}
//*************************** 同步接口 end ***********************************//

//************************** 异步接口 begin **********************************//

/**
 * @brief 异步启动一次寄存器读。
 *
 * Steps:
 *  1. 保存完成回调并加锁。
 *  2. 启动 DMA 后立刻解锁，完成时在中断里回调。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[out] p_data               : 接收缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  pf_done              : 完成回调，可为空（改为靠信号量通知）。
 * @param[in]  p_user_data          : 回调上下文。
 *
 * @return en_core_i2c_status_t : CORE_I2C_OK 只代表启动成功。
 */
en_core_i2c_status_t core_i2c_read_async(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size,
    core_i2c_done_callback_t     pf_done,
    void                        *p_user_data)
{
    en_core_i2c_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    if ((NULL == pf_done) && (NULL == g_i2c_os[port].sema))
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    g_i2c_os[port].pf_done = pf_done;
    g_i2c_os[port].p_user_data = p_user_data;
    g_i2c_os[port].device_address_7bit = device_address_7bit;
    g_i2c_os[port].status = CORE_I2C_ERROR;
    g_i2c_os[port].error_code = 0U;

    status = i2c_port_os_lock(g_i2c_os[port].mutex, 0U);
    if (CORE_I2C_OK != status)
    {
        g_i2c_os[port].pf_done = NULL;
        return status;
    }

    status = i2c_bus_mem_read_dma(port,
                                  device_address_7bit,
                                  memory_address,
                                  address_size,
                                  p_data,
                                  size);

    i2c_port_os_unlock(g_i2c_os[port].mutex);

    if (CORE_I2C_OK != status)
    {
        g_i2c_os[port].pf_done = NULL;
    }

    return status;
}

/**
 * @brief 异步启动一次寄存器写。
 *
 * Steps:
 *  1. 保存完成回调并加锁。
 *  2. 启动 DMA 后立刻解锁，完成时在中断里回调。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[in]  p_data               : 发送缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  pf_done              : 完成回调，可为空（改为靠信号量通知）。
 * @param[in]  p_user_data          : 回调上下文。
 *
 * @return en_core_i2c_status_t : CORE_I2C_OK 只代表启动成功。
 */
en_core_i2c_status_t core_i2c_write_async(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *p_data,
    uint16_t                     size,
    core_i2c_done_callback_t     pf_done,
    void                        *p_user_data)
{
    en_core_i2c_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    if ((NULL == pf_done) && (NULL == g_i2c_os[port].sema))
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    g_i2c_os[port].pf_done = pf_done;
    g_i2c_os[port].p_user_data = p_user_data;
    g_i2c_os[port].device_address_7bit = device_address_7bit;
    g_i2c_os[port].status = CORE_I2C_ERROR;
    g_i2c_os[port].error_code = 0U;

    status = i2c_port_os_lock(g_i2c_os[port].mutex, 0U);
    if (CORE_I2C_OK != status)
    {
        g_i2c_os[port].pf_done = NULL;
        return status;
    }

    status = i2c_bus_mem_write_dma(port,
                                   device_address_7bit,
                                   memory_address,
                                   address_size,
                                   p_data,
                                   size);

    i2c_port_os_unlock(g_i2c_os[port].mutex);

    if (CORE_I2C_OK != status)
    {
        g_i2c_os[port].pf_done = NULL;
    }

    return status;
}
//*************************** 异步接口 end ***********************************//

//************************** 辅助接口 begin **********************************//

/**
 * @brief 中止指定总线上的 DMA 传输。
 *
 * Steps:
 *  1. 检查槽位范围。
 *  2. 转发给 Bus 发送 STOP 并等中止完成中断。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return en_core_i2c_status_t : 中止请求结果。
 */
en_core_i2c_status_t core_i2c_abort(core_i2c_port_id_t port)
{
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    return i2c_bus_abort_dma(port);
}

/**
 * @brief 读取最近一次传输的 HAL 原始错误码。
 *
 * 步骤：
 *  1. 检查槽位范围。
 *  2. 返回保存的错误码，0 表示无错误。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return uint32_t : HAL 原始错误码。
 */
uint32_t core_i2c_get_error_code(core_i2c_port_id_t port)
{
    if (port >= CORE_I2C_PORT_MAX)
    {
        return 0U;
    }

    return g_i2c_os[port].error_code;
}
//*************************** 辅助接口 end ***********************************//

//****************** Function Definitions end ********************************//

//******************************* End of File ********************************//
