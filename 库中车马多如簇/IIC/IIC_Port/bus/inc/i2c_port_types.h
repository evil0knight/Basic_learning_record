/******************************************************************************
 * Copyright (C) 2026 YA.
 *
 * All Rights Reserved.
 *
 * @file i2c_port_types.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author YA
 *
 * @brief 提供不依赖 MCU HAL 的 I2C 公共类型。
 *
 * Processing flow:
 *
 * 平台接口和设备驱动共享传输状态、端口、回调及寄存器地址宽度。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef I2C_PORT_TYPES_H
#define I2C_PORT_TYPES_H

//****************************** Includes begin ******************************//
#include <stdint.h>
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
/* 每条总线的请求队列深度，满了由 core_i2c_submit 返回 BUSY。 */
#ifndef CORE_I2C_QUEUE_DEPTH
#define CORE_I2C_QUEUE_DEPTH 4U
#endif
//******************************* Defines end ********************************//

//******************************* Enums begin ********************************//
typedef enum
{
    CORE_I2C_OK = 0,
    CORE_I2C_ERROR,
    CORE_I2C_ERR_ARGUMENT,
    CORE_I2C_ERR_TIMEOUT,
    CORE_I2C_ERR_BUSY,
    CORE_I2C_ERR_NOT_CONFIGURED,
    CORE_I2C_ERR_NACK,
    CORE_I2C_ERR_UNSUPPORTED,
    CORE_I2C_PENDING
} en_core_i2c_status_t;    // I2C 操作统一状态码

typedef enum
{
    CORE_I2C_MEM_ADDR_8BIT = 1,
    CORE_I2C_MEM_ADDR_16BIT = 2
} en_core_i2c_mem_addr_size_t;    // 设备内部寄存器地址宽度

typedef enum
{
    CORE_I2C_PORT_ENCODER1 = 0,    // 编码器所在的逻辑总线
    CORE_I2C_PORT_MAX = 8          // 槽位上限，留出扩容余量
} core_i2c_port_id_t;    // 逻辑 I2C 总线槽位编号
//******************************** Enums end *********************************//

//****************************** Typedefs begin ******************************//
typedef void (*i2c_rx_callback_t)(    // 读完成回调
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    en_core_i2c_status_t  status,
    uint8_t              *p_data,
    uint16_t              size);

typedef void (*i2c_tx_callback_t)(    // 写完成回调
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    en_core_i2c_status_t  status);

typedef void (*i2c_error_callback_t)(    // 错误回调
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    uint32_t              error_code);

typedef void (*core_i2c_done_callback_t)(    // Port 异步完成回调
    core_i2c_port_id_t    port,
    uint8_t               device_address_7bit,
    en_core_i2c_status_t  status,
    void                 *p_user_data);

typedef struct
{
    core_i2c_port_id_t           port;                // 逻辑总线槽位
    uint8_t                      device_address_7bit; // 七位设备地址
    uint16_t                     memory_address;      // 内部寄存器地址
    en_core_i2c_mem_addr_size_t  address_size;        // 寄存器地址宽度
    uint8_t                     *p_data;              // 收发缓冲区
    uint16_t                     size;                // 字节数
    uint8_t                      is_write;            // 非零表示写
    uint32_t                     timeout_ms;          // 0 表示不限时
    core_i2c_done_callback_t     pf_done;             // 完成回调，可为 NULL
    void                        *p_user_data;         // 回调上下文

    /* 每笔请求自己的结果，由 Port 在完成时回写。
     * 调用方提交前置 done = 0；读到 done = 1 后自行清 0。
     * 这样队列里排多笔时，结果不会互相覆盖。 */
    volatile uint8_t              done;               // 0 = 未完成
    volatile en_core_i2c_status_t status;             // 本笔的完成状态
} core_i2c_request_t;    // 一次排队中的总线请求
//******************************* Typedefs end *******************************//

#endif /* I2C_PORT_TYPES_H */

//******************************* End of File ********************************//
