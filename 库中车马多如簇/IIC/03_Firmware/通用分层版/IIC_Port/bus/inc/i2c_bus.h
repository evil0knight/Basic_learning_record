/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file i2c_bus.h
 *
 * @par dependencies
 * - i2c_port_types.h
 * - main.h
 *
 * @author YA
 *
 * @brief 提供阻塞与异步 DMA 的 I2C 协议接口。
 *
 * 处理流程：
 * 配置槽位后，通过 Bus 接口完成参数检查、传输和完成分发；
 * 锁、超时、同步等待由 Port 层负责。
 *
 * @version V1.0
 *
 * @note 四个空格缩进，UTF-8 编码。
 *
 *****************************************************************************/

#ifndef I2C_BUS_H
#define I2C_BUS_H

//**************************** Defines begin *********************************//
/* 默认启用硬件 I2C；只用软件 I2C 时在工程宏中定义为 0。 */
#ifndef CORE_I2C_HW_ENABLE
#define CORE_I2C_HW_ENABLE 1
#endif

/* abort 后等 AbortCplt 的兜底超时，超时则强制复位总线。 */
#ifndef CORE_I2C_ABORT_TIMEOUT_MS
#define CORE_I2C_ABORT_TIMEOUT_MS 5U
#endif
//***************************** Defines end **********************************//

//**************************** Includes begin ********************************//
#include "i2c_port_types.h"

#include "main.h"
//****************************** Includes end ********************************//

//************************** Usage Note begin ********************************//
/* 同步等待效果由所选 Port 决定：
 * - 裸机 Port：_sync 忙等，占 CPU，不是死锁；
 * - RTOS Port：_sync 用信号量阻塞，不占 CPU；
 * - timeout_ms == 0 表示一直等，可能卡死，建议传非 0。
 * 控制循环优先用非阻塞 read。
 */
//*************************** Usage Note end *********************************//

//**************************** Typedefs begin ********************************//
#if (CORE_I2C_HW_ENABLE != 0)
typedef struct
{
    I2C_HandleTypeDef *handle;    // 硬件 I2C 句柄
} st_core_hardware_i2c_config_t;
#endif

typedef struct
{
    GPIO_TypeDef         *scl_port;       // SCL 在哪个口
    uint16_t              scl_pin;        // SCL 引脚号
    GPIO_TypeDef         *sda_port;       // SDA 在哪个口
    uint16_t              sda_pin;        // SDA 引脚号
    uint32_t              half_period_us; // 半周期，单位 us
    void                (*delay_us)(uint32_t delay_us); // 微秒延时
} st_core_software_i2c_config_t;
//****************************** Typedefs end ********************************//

//***************** Function Prototypes begin ********************************//
/**
 * @brief 初始化 I2C Bus 的软件状态。
 */
en_core_i2c_status_t i2c_bus_init(void);

/**
 * @brief 查询逻辑总线是否正在传输。
 */
uint8_t i2c_bus_is_busy(core_i2c_port_id_t port);

/**
 * @brief 查询逻辑总线是否为软件 I2C 槽位。
 */
uint8_t i2c_bus_is_software(core_i2c_port_id_t port);

#if (CORE_I2C_HW_ENABLE != 0)
/**
 * @brief 获取逻辑总线对应的 HAL I2C 句柄。
 */
I2C_HandleTypeDef *i2c_bus_get_handle(core_i2c_port_id_t port);
#endif

//**************************** 死等接口 begin *******************************//
/* 阻塞接口：忙等。
 * 硬件槽位为 HAL 轮询，软件槽位为 GPIO 位翻转；
 * 返回时传输已结束，适合初始化/低频访问。
 * i2c_bus_read 直接读；i2c_bus_mem_read 先写寄存器地址再读。
 */
/**
 * @brief 向设备写入连续数据。
 */
en_core_i2c_status_t i2c_bus_write(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit,
    const uint8_t      *data,
    uint16_t            size,
    uint32_t            timeout_ms);

/**
 * @brief 从设备读取连续数据。
 */
en_core_i2c_status_t i2c_bus_read(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit,
    uint8_t            *data,
    uint16_t            size,
    uint32_t            timeout_ms);

/**
 * @brief 向设备指定内部地址写入数据。
 */
en_core_i2c_status_t i2c_bus_mem_write(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *data,
    uint16_t                     size,
    uint32_t                     timeout_ms);

/**
 * @brief 从设备指定内部地址读取数据。
 */
en_core_i2c_status_t i2c_bus_mem_read(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *data,
    uint16_t                     size,
    uint32_t                     timeout_ms);

/**
 * @brief 探测设备是否应答其地址。
 */
en_core_i2c_status_t i2c_bus_is_ready(
    core_i2c_port_id_t  port,
    uint8_t             device_address_7bit,
    uint32_t            trials,
    uint32_t            timeout_ms);
//***************************** 死等接口 end ********************************//


//************************** DMA 接口 begin *********************************//
/* 异步接口：启动 DMA 后立即返回。
 * 返回 CORE_I2C_OK 只代表启动成功，完成通过回调通知。
 */
/**
 * @brief 异步启动一次指定内部地址的 DMA 读。
 */
en_core_i2c_status_t i2c_bus_mem_read_dma(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    uint8_t                     *p_data,
    uint16_t                     size);

/**
 * @brief 异步启动一次指定内部地址的 DMA 写。
 */
en_core_i2c_status_t i2c_bus_mem_write_dma(
    core_i2c_port_id_t           port,
    uint8_t                      device_address_7bit,
    uint16_t                     memory_address,
    en_core_i2c_mem_addr_size_t  address_size,
    const uint8_t               *p_data,
    uint16_t                     size);

/**
 * @brief 中止当前 DMA 传输。
 */
en_core_i2c_status_t i2c_bus_abort_dma(core_i2c_port_id_t port);

/**
 * @brief 强制复位总线状态，用于 abort 完成中断丢失后的兜底恢复。
 */
en_core_i2c_status_t i2c_bus_recover(core_i2c_port_id_t port);
//*************************** DMA 接口 end **********************************//

//*************************** Callbacks begin ********************************//
/* 回调接口：完成/错误通知，在中断上下文执行。 */
/**
 * @brief 注册读完成回调。
 */
en_core_i2c_status_t i2c_bus_register_rx_callback(
    core_i2c_port_id_t port,
    i2c_rx_callback_t  pf_callback);

/**
 * @brief 注册写完成回调。
 */
en_core_i2c_status_t i2c_bus_register_tx_callback(
    core_i2c_port_id_t port,
    i2c_tx_callback_t  pf_callback);

/**
 * @brief 注册错误回调。
 */
en_core_i2c_status_t i2c_bus_register_error_callback(
    core_i2c_port_id_t    port,
    i2c_error_callback_t  pf_callback);
//**************************** Callbacks end *********************************//

//*************************** IRQ Forwarding begin ***************************//
/* 中断转发：仅供 App ISR Handler 调用。 */
/**
 * @brief 转发 HAL 内存读完成事件。
 */
en_core_i2c_status_t i2c_bus_irq_mem_rx_complete(
    I2C_HandleTypeDef *p_handle);

/**
 * @brief 转发 HAL 内存写完成事件。
 */
en_core_i2c_status_t i2c_bus_irq_mem_tx_complete(
    I2C_HandleTypeDef *p_handle);

/**
 * @brief 转发 HAL I2C 错误事件。
 */
en_core_i2c_status_t i2c_bus_irq_error(
    I2C_HandleTypeDef *p_handle);

/**
 * @brief 转发 HAL I2C 中止完成事件。
 */
en_core_i2c_status_t i2c_bus_irq_abort_complete(
    I2C_HandleTypeDef *p_handle);
//**************************** IRQ Forwarding end ****************************//

//******************* Function Prototypes end ********************************//

#endif /* I2C_BUS_H */

//******************************* End of File ********************************//
