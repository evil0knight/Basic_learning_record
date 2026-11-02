/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file i2c_port_bare.c
 *
 * @par dependencies
 * - i2c_port_bare.h
 * - i2c_bus.h
 * - critical_port.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 裸机 I2C Port 实现，请求队列加硬件超时，全部由中断驱动。
 *
 * Processing flow:
 *
 * submit 入队并启动 DMA 与超时定时器
 * -> DMA 完成中断或定时器中断先到，都进 i2c_port_bare_dispatch 分发
 * -> 分发完自动启动队列里的下一个。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "i2c_port_bare.h"

#include "../../bus/inc/i2c_bus.h"

#include "../../../critical/inc/critical_port.h"
#include "../../../time/inc/time_port.h"

#include <stddef.h>
//******************************* Includes end *******************************//

//**************************** Defines begin *********************************//
/* 超时定时器按 1 MHz 计数，1 tick == 1 us。 */
#define I2C_PORT_BARE_US_PER_MS 1000U
//***************************** Defines end **********************************//

//**************************** Typedefs begin ********************************//
/* 每个逻辑总线槽位一份：待处理队列、在飞请求、超时起点、故障状态。 */
typedef struct
{
    core_i2c_request_t *req[CORE_I2C_QUEUE_DEPTH];  // 待处理请求（存指针）
    volatile uint8_t    head;                       // 队首下标
    volatile uint8_t    count;                      // 待处理个数
    core_i2c_request_t *cur;                        // 在飞请求（存指针）
    volatile uint8_t    cur_busy;                   // 在飞标志
    volatile uint8_t    skip_done;                  // 丢弃 abort 迟到中断
    uint32_t            cur_tick;                   // 在飞起点
    volatile uint32_t   last_error;                 // HAL 原始错误码
} st_i2c_port_bare_t;
//****************************** Typedefs end ********************************//

//************************** Static Variables begin **************************//
static st_i2c_port_bare_t g_i2c_bare[CORE_I2C_PORT_MAX];

/* 同步接口每端口一笔请求，结果回写在请求自己的 done/status 里。 */
static core_i2c_request_t g_i2c_sync_req[CORE_I2C_PORT_MAX];
static volatile uint8_t   g_i2c_sync_started[CORE_I2C_PORT_MAX];
static uint32_t           g_i2c_sync_tick[CORE_I2C_PORT_MAX];
//*************************** Static Variables end ***************************//

//************************** Static Functions begin **************************//
/**
 * @brief 队首请求出队。
 *
 * 步骤：
 *  1. 队列非空时头指针循环前移。
 *  2. 待处理计数减一。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return void : 无返回值。
 */
static void i2c_port_bare_pop(core_i2c_port_id_t port)
{
    st_i2c_port_bare_t *bus = &g_i2c_bare[port];

    if (0U == bus->count)
    {
        return;
    }

    bus->head = (uint8_t)((bus->head + 1U) % CORE_I2C_QUEUE_DEPTH);
    bus->count--;
}

/**
 * @brief 判断同一器件是否已有请求在飞或排队。
 *
 * 步骤：
 *  1. 检查在飞请求的设备地址。
 *  2. 遍历队列检查设备地址。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 *
 * @return uint8_t : 非零表示该器件已有未完成请求。
 */
static uint8_t i2c_port_bare_has_pending(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit)
{
    st_i2c_port_bare_t *bus = &g_i2c_bare[port];
    uint8_t             i;
    uint8_t             index;

    if ((0U != bus->cur_busy) &&
        (bus->cur->device_address_7bit == device_address_7bit))
    {
        return 1U;
    }

    for (i = 0U; i < bus->count; ++i)
    {
        index = (uint8_t)((bus->head + i) % CORE_I2C_QUEUE_DEPTH);
        if (bus->req[index]->device_address_7bit == device_address_7bit)
        {
            return 1U;
        }
    }

    return 0U;
}

/**
 * @brief 总线空闲时启动队首请求。
 *
 * 步骤：
 *  1. 临界区内确认总线空闲且队列非空。
 *  2. 硬件槽位启动 DMA 和超时定时器；软件槽位就地做完。
 *  3. 完成后出队，失败则回调提交方。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return void : 无返回值。
 */
static void i2c_port_bare_kick(core_i2c_port_id_t port)
{
    st_i2c_port_bare_t       *bus = &g_i2c_bare[port];
    en_core_i2c_status_t      status;
    core_i2c_done_callback_t  pf_done;
    void                     *p_user_data;
    uint8_t                   device_address;
    uint8_t                   need_report = 0U;
    uint32_t                  irq_state;
    core_i2c_request_t       *p_req = NULL;

    (void)platform_critical_enter(&irq_state);

    if ((0U != bus->cur_busy) || (0U == bus->count) ||
        (0U != i2c_bus_is_busy(port)))
    {
        (void)platform_critical_exit(irq_state);
        return;
    }

    bus->cur = bus->req[bus->head];
    bus->cur_busy = 1U;
    i2c_port_bare_pop(port);
    bus->last_error = 0U;

    if (0U != i2c_bus_is_software(port))
    {
        /* 软件槽位没有 DMA，就地做完。 */
        status = (0U != bus->cur->is_write)
                     ? i2c_bus_mem_write(port,
                                         bus->cur->device_address_7bit,
                                         bus->cur->memory_address,
                                         bus->cur->address_size,
                                         bus->cur->p_data,
                                         bus->cur->size,
                                         bus->cur->timeout_ms)
                     : i2c_bus_mem_read(port,
                                        bus->cur->device_address_7bit,
                                        bus->cur->memory_address,
                                        bus->cur->address_size,
                                        bus->cur->p_data,
                                        bus->cur->size,
                                        bus->cur->timeout_ms);
        p_req = bus->cur;
        bus->cur = NULL;
        bus->cur_busy = 0U;
        pf_done = p_req->pf_done;
        p_user_data = p_req->p_user_data;
        device_address = p_req->device_address_7bit;
        need_report = 1U;
    }
    else
    {
        status = (0U != bus->cur->is_write)
                     ? i2c_bus_mem_write_dma(port,
                                             bus->cur->device_address_7bit,
                                             bus->cur->memory_address,
                                             bus->cur->address_size,
                                             bus->cur->p_data,
                                             bus->cur->size)
                     : i2c_bus_mem_read_dma(port,
                                            bus->cur->device_address_7bit,
                                            bus->cur->memory_address,
                                            bus->cur->address_size,
                                            bus->cur->p_data,
                                            bus->cur->size);
        if (CORE_I2C_OK == status)
        {
            bus->cur_tick = core_time_get();
        }
        else
        {
            p_req = bus->cur;
            bus->cur = NULL;
            bus->cur_busy = 0U;
            pf_done = p_req->pf_done;
            p_user_data = p_req->p_user_data;
            device_address = p_req->device_address_7bit;
            need_report = 1U;
        }
    }

    (void)platform_critical_exit(irq_state);

    if (0U != need_report)
    {
        /* 结果回写到这一笔自己的格子。 */
        p_req->status = status;
        p_req->done = 1U;

        if (NULL != pf_done)
        {
            pf_done(port, device_address, status, p_user_data);
        }
        i2c_port_bare_kick(port);
    }
}

/**
 * @brief 统一分发函数，DMA 完成和超时中断都走这里。
 *
 * 步骤：
 *  1. 停超时定时器。
 *  2. 没有在飞请求时直接返回。
 *  3. 清在飞标志，回调提交方，再启动下一个请求。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  status               : 本次传输结果。
 *
 * @return void : 无返回值。
 */
static void i2c_port_bare_dispatch(
    core_i2c_port_id_t    port,
    en_core_i2c_status_t  status)
{
    st_i2c_port_bare_t       *bus = &g_i2c_bare[port];
    core_i2c_done_callback_t  pf_done;
    void                     *p_user_data;
    uint8_t                   device_address;
    core_i2c_request_t       *p_req;

    if (0U == bus->cur_busy)
    {
        return;
    }

    p_req = bus->cur;
    bus->cur = NULL;
    bus->cur_busy = 0U;

    if (NULL == p_req)
    {
        return;
    }

    /* 结果回写到这一笔自己的格子，多笔排队时不会互相覆盖。 */
    p_req->status = status;
    p_req->done = 1U;

    pf_done = p_req->pf_done;
    p_user_data = p_req->p_user_data;
    device_address = p_req->device_address_7bit;

    if (NULL != pf_done)
    {
        pf_done(port, device_address, status, p_user_data);
    }

    i2c_port_bare_kick(port);
}

/**
 * @brief 读完成回调，转入统一分发。
 *
 * 步骤：
 *  1. 检查槽位范围。
 *  2. 有在飞请求时分发结果。
 *  3. 否则清掉 abort 迟到标志。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  status               : 本次传输结果。
 * @param[in]  p_data               : 接收缓冲区。
 * @param[in]  size                 : 接收字节数。
 *
 * @return void : 无返回值。
 */
static void i2c_port_bare_rx_cb(
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    en_core_i2c_status_t  status,
    uint8_t              *p_data,
    uint16_t              size)
{
    (void)device_address_7bit;
    (void)p_data;
    (void)size;

    if (port >= CORE_I2C_PORT_MAX)
    {
        return;
    }

    if (0U != g_i2c_bare[port].cur_busy)
    {
        i2c_port_bare_dispatch(port, status);
        return;
    }

    if (0U != g_i2c_bare[port].skip_done)
    {
        g_i2c_bare[port].skip_done = 0U;
        i2c_port_bare_kick(port);
    }
}

/**
 * @brief 写完成回调，转入统一分发。
 *
 * 步骤：
 *  1. 检查槽位范围。
 *  2. 有在飞请求时分发结果。
 *  3. 否则清掉 abort 迟到标志。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  status               : 本次传输结果。
 *
 * @return void : 无返回值。
 */
static void i2c_port_bare_tx_cb(
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    en_core_i2c_status_t  status)
{
    (void)device_address_7bit;

    if (port >= CORE_I2C_PORT_MAX)
    {
        return;
    }

    if (0U != g_i2c_bare[port].cur_busy)
    {
        i2c_port_bare_dispatch(port, status);
        return;
    }

    if (0U != g_i2c_bare[port].skip_done)
    {
        g_i2c_bare[port].skip_done = 0U;
        i2c_port_bare_kick(port);
    }
}

/**
 * @brief 错误回调，只记录 HAL 原始错误码。
 *
 * 步骤：
 *  1. 检查槽位范围。
 *  2. 保存 HAL 错误码，供诊断使用。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  error_code           : HAL 原始错误码。
 *
 * @return void : 无返回值。
 */
static void i2c_port_bare_error_cb(
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    uint32_t              error_code)
{
    (void)device_address_7bit;

    if (port >= CORE_I2C_PORT_MAX)
    {
        return;
    }

    g_i2c_bare[port].last_error = error_code;
}

/**
 * @brief 普通 HAL 收发一次，CPU 在 HAL 内部轮询。
 *
 * 步骤：
 *  1. 软件槽位用临界区包住位翻转。
 *  2. 硬件槽位直接调普通 HAL。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[in]  p_data               : 收发缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  timeout_ms           : 普通 HAL 的超时，0 表示不限时。
 * @param[in]  is_write             : 非零表示写。
 *
 * @return en_core_i2c_status_t : 传输结果。
 */
static en_core_i2c_status_t i2c_port_bare_plain(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms,
    uint8_t                      is_write)
{
    en_core_i2c_status_t status;
    uint32_t             irq_state;

    if (0U == i2c_bus_is_software(port))
    {
        return (0U != is_write)
                   ? i2c_bus_mem_write(port, device_address_7bit,
                                       memory_address, address_size,
                                       p_data, size, timeout_ms)
                   : i2c_bus_mem_read(port, device_address_7bit,
                                      memory_address, address_size,
                                      p_data, size, timeout_ms);
    }

    (void)platform_critical_enter(&irq_state);
    status = (0U != is_write)
                 ? i2c_bus_mem_write(port, device_address_7bit,
                                     memory_address, address_size,
                                     p_data, size, timeout_ms)
                 : i2c_bus_mem_read(port, device_address_7bit,
                                    memory_address, address_size,
                                    p_data, size, timeout_ms);
    (void)platform_critical_exit(irq_state);

    return status;
}
/**
 * @brief 提交一笔请求后立刻返回，下次调用再来收结果。
 *
 * 步骤：
 *  1. 上一笔已完成：取走结果并返回。
 *  2. 上一笔仍在跑：判超时，未超时返回 CORE_I2C_PENDING。
 *  3. 没有在飞请求：组装请求提交，返回 CORE_I2C_PENDING。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 * @param[in]  device_address_7bit  : 未左移的七位设备地址。
 * @param[in]  memory_address       : 设备内部寄存器或存储地址。
 * @param[in]  address_size         : 内部地址宽度。
 * @param[in]  p_data               : 收发缓冲区。
 * @param[in]  size                 : 传输字节数。
 * @param[in]  timeout_ms           : 传输阶段超时，0 表示不限时。
 * @param[in]  is_write             : 非零表示写。
 *
 * @return en_core_i2c_status_t : CORE_I2C_PENDING 表示还没完成。
 */
static en_core_i2c_status_t i2c_port_bare_wait(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms,
    uint8_t                      is_write)
{
    core_i2c_request_t   *request;
    en_core_i2c_status_t  status;

    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }

    request = &g_i2c_sync_req[port];

    /* 1. 上一笔已完成：取走结果，调用方自行清 done。 */
    if (0U != request->done)
    {
        status = request->status;
        request->done = 0U;
        g_i2c_sync_started[port] = 0U;
        return status;
    }

    /* 2. 上一笔还在跑：查一次时间，超时就 abort。 */
    if (0U != g_i2c_sync_started[port])
    {
        if ((0U != timeout_ms) &&
            ((core_time_get() - g_i2c_sync_tick[port]) >= timeout_ms))
        {
            (void)i2c_bus_abort_dma(port);
            g_i2c_sync_started[port] = 0U;
            request->done = 0U;
            return CORE_I2C_ERR_TIMEOUT;
        }

        return CORE_I2C_PENDING;
    }

    /* 3. 没有在飞请求：提交一笔，立刻返回。 */
    request->port = port;
    request->device_address_7bit = device_address_7bit;
    request->memory_address = memory_address;
    request->address_size = address_size;
    request->p_data = p_data;
    request->size = size;
    request->is_write = is_write;
    request->timeout_ms = timeout_ms;
    request->pf_done = NULL;
    request->p_user_data = NULL;
    request->done = 0U;
    request->status = CORE_I2C_PENDING;

    status = core_i2c_submit(request);
    if (CORE_I2C_OK != status)
    {
        /* 队列满或有同类请求：这一轮不发，下一轮再来。 */
        return status;
    }

    g_i2c_sync_tick[port] = core_time_get();
    g_i2c_sync_started[port] = 1U;

    /* 启动阶段就失败时 done 已经置 1，直接取走结果。 */
    if (0U != request->done)
    {
        status = request->status;
        request->done = 0U;
        g_i2c_sync_started[port] = 0U;
        return status;
    }

    return CORE_I2C_PENDING;
}
/**
 * @brief 在飞请求超时就 abort 并按 TIMEOUT 分发。
 *
 * 步骤：
 *  1. 没有在飞请求或没设超时直接返回。
 *  2. 用当前时间和起点比较，超时则 abort 并分发。
 *
 * @param[in]  port                 : 逻辑总线槽位索引。
 *
 * @return void : 无返回值。
 */
static void i2c_port_bare_check_timeout(core_i2c_port_id_t port)
{
    st_i2c_port_bare_t *bus = &g_i2c_bare[port];

    if ((0U == bus->cur_busy) || (0U == bus->cur->timeout_ms))
    {
        return;
    }

    if ((core_time_get() - bus->cur_tick) < bus->cur->timeout_ms)
    {
        return;
    }

    (void)i2c_bus_abort_dma(port);
    bus->skip_done = 1U;
    i2c_port_bare_dispatch(port, CORE_I2C_ERR_TIMEOUT);
}

//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//

//************************** 公共接口 begin **********************************//

/**
 * @brief 初始化 Bus 并注册 Port 的读、写、错误回调。
 *
 * Steps:
 *  1. 清空 Bus 槽位的 busy 与回调表。
 *  2. 清空每个槽位的队列、定时器和故障状态。
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
        g_i2c_bare[port].head = 0U;
        g_i2c_bare[port].count = 0U;
        g_i2c_bare[port].cur = NULL;
        g_i2c_bare[port].cur_busy = 0U;
        g_i2c_bare[port].skip_done = 0U;
        g_i2c_bare[port].last_error = 0U;

        g_i2c_sync_req[port].done = 0U;
        g_i2c_sync_req[port].status = CORE_I2C_PENDING;
        g_i2c_sync_started[port] = 0U;

        (void)i2c_bus_register_rx_callback(port, i2c_port_bare_rx_cb);
        (void)i2c_bus_register_tx_callback(port, i2c_port_bare_tx_cb);
        (void)i2c_bus_register_error_callback(port, i2c_port_bare_error_cb);
    }

    return CORE_I2C_OK;
}
//*************************** 公共接口 end ***********************************//

//************************** 死等接口 begin **********************************//

/**
 * @brief 普通 HAL 读寄存器，CPU 在 HAL 内部轮询。
 *
 * Steps:
 *  1. 检查槽位范围。
 *  2. 直接调 Bus 的普通读接口。
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
en_core_i2c_status_t core_i2c_mem_read(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms)
{
    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return i2c_port_bare_plain(port, device_address_7bit, memory_address,
                               address_size, p_data, size, timeout_ms, 0U);
}

/**
 * @brief 普通 HAL 写寄存器，CPU 在 HAL 内部轮询。
 *
 * Steps:
 *  1. 检查槽位范围。
 *  2. 直接调 Bus 的普通写接口。
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
en_core_i2c_status_t core_i2c_mem_write(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *p_data,
    uint16_t                     size,
    uint32_t                     timeout_ms)
{
    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return i2c_port_bare_plain(port, device_address_7bit, memory_address,
                               address_size, (uint8_t *)p_data, size,
                               timeout_ms, 1U);
}
//*************************** 死等接口 end ***********************************//

//************************** 同步接口 begin **********************************//

/**
 * @brief DMA 读寄存器，本函数内等自己的完成标志位。
 *
 * Steps:
 *  1. 检查槽位范围。
 *  2. 提交请求后 WFI 等结果。
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
    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return i2c_port_bare_wait(port, device_address_7bit, memory_address,
                              address_size, p_data, size, timeout_ms, 0U);
}

/**
 * @brief DMA 写寄存器，本函数内等自己的完成标志位。
 *
 * Steps:
 *  1. 检查槽位范围。
 *  2. 提交请求后 WFI 等结果。
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
    //-------------------------参数检查 begin-------------------------//
    if (port >= CORE_I2C_PORT_MAX)
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return i2c_port_bare_wait(port, device_address_7bit, memory_address,
                              address_size, (uint8_t *)p_data, size,
                              timeout_ms, 1U);
}
//*************************** 同步接口 end ***********************************//

//************************** 异步接口 begin **********************************//

/**
 * @brief 把一次请求放进指定总线的队列。
 *
 * Steps:
 *  1. 检查请求字段。
 *  2. 同一器件已有未完成请求时返回 BUSY，避免单个 BSP 占满队列。
 *  3. 临界区内写入队尾，出临界区后尝试立刻启动。
 *
 * @param[in]  p_request            : 待提交的请求，Port 会回写其 done/status。
 *
 * @return en_core_i2c_status_t : 队列满或器件重复返回 CORE_I2C_ERR_BUSY。
 */
en_core_i2c_status_t core_i2c_submit(core_i2c_request_t *p_request)
{
    st_i2c_port_bare_t   *bus;
    en_core_i2c_status_t  status;
    uint32_t              irq_state;
    uint8_t               slot;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_request) || (p_request->port >= CORE_I2C_PORT_MAX) ||
        (NULL == p_request->p_data) || (0U == p_request->size) ||
        (p_request->device_address_7bit > 0x7FU))
    {
        return CORE_I2C_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    bus = &g_i2c_bare[p_request->port];
    status = CORE_I2C_OK;

    /* 没有回调的请求没人计时，借下一次提交顺带检查在飞请求是否超时。 */
    i2c_port_bare_check_timeout(p_request->port);

    (void)platform_critical_enter(&irq_state);
    if (0U != i2c_port_bare_has_pending(p_request->port,
                                        p_request->device_address_7bit))
    {
        status = CORE_I2C_ERR_BUSY;
    }
    else if (bus->count >= CORE_I2C_QUEUE_DEPTH)
    {
        status = CORE_I2C_ERR_BUSY;
    }
    else
    {
        slot = (uint8_t)((bus->head + bus->count) % CORE_I2C_QUEUE_DEPTH);
        bus->req[slot] = p_request;
        bus->count++;
    }
    (void)platform_critical_exit(irq_state);

    if (CORE_I2C_OK == status)
    {
        i2c_port_bare_kick(p_request->port);
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
 * Steps:
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

    return g_i2c_bare[port].last_error;
}
//*************************** 辅助接口 end ***********************************//

//****************** Function Definitions end ********************************//

//******************************* End of File ********************************//
