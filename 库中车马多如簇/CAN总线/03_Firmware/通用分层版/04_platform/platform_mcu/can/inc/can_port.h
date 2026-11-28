/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file can_port.h
 *
 * @par dependencies
 * - stdint.h
 * - main.h
 * - fdcan.h 或 can.h
 *
 * @author YA
 *
 * @brief 多实例经典 CAN / CAN FD 平台接口。
 *
 * Processing flow:
 *
 * 参数检查 -> 调用 HAL CAN 或分发回调 -> 状态码转换 -> 返回。
 *
 * @version V1.0 2026-11-23
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

#ifndef CAN_PORT_H
#define CAN_PORT_H

//****************************** Includes begin ******************************//
#include <stdint.h>
#include "main.h"
/* 工程可通过编译宏选择后端；main.h 也可统一定义该宏。 */
#ifndef CORE_CAN_USE_FDCAN
#define CORE_CAN_USE_FDCAN (1U)
#endif
#if CORE_CAN_USE_FDCAN
#include "fdcan.h"
#else
#include "can.h"
#endif
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
/* 应用传真实字节数，HAL DLC 编码由实现转换。 */
#define CORE_CAN_DATA_MAX         (64U)
#define CORE_CAN_TX_TIMEOUT_MS    (10U)
#define CORE_CAN_RX_TIMEOUT_MS    (10U)
//******************************* Defines end ********************************//

//******************************* Enums begin ********************************//
typedef enum
{
    CORE_CAN_OK = 0,
    CORE_CAN_ERROR,
    CORE_CAN_ERR_ARGUMENT,
    CORE_CAN_ERR_BUSY,
    CORE_CAN_ERR_TIMEOUT,
    CORE_CAN_ERR_NOT_READY
} en_core_can_status_t;

typedef enum
{
    CORE_CAN_FIFO0 = 0,
    CORE_CAN_FIFO1
} en_core_can_fifo_t;

typedef enum
{
    CORE_CAN1 = 0,
    CORE_CAN_MAX
} en_core_can_instance_t;
//******************************** Enums end *********************************//

//****************************** Typedefs begin ******************************//
#if CORE_CAN_USE_FDCAN
typedef FDCAN_HandleTypeDef st_can_handle_t;
#else
typedef CAN_HandleTypeDef st_can_handle_t;
#endif
//******************************* Typedefs end *******************************//

//****************************** Structs begin *******************************//
/** @brief 统一经典 CAN / CAN FD 帧；id 为右对齐的 11/29 位 ID。 */
typedef struct
{
    uint32_t id;
    uint8_t is_ext; /* 0 标准帧，1 扩展帧。 */
    uint8_t is_rtr; /* 0 数据帧，1 遥控帧。 */
    uint8_t is_fd;  /* 0 经典帧，1 FD 帧。 */
    uint8_t is_brs; /* 仅 FD 可为 1，表示数据段切换速率。 */
    uint8_t dlc;    /* 字节数：0~8、12、16、20、24、32、48、64。 */
    uint8_t data[CORE_CAN_DATA_MAX];
} st_can_frame_t;

typedef struct
{
    st_can_handle_t *p_handle;
    uint32_t timeout; /* 板级默认超时；显式传入的 timeout_ms 优先。 */
} st_can_config_t;
//******************************* Structs end ********************************//

//****************************** Typedefs begin ******************************//
/* 回调在 ISR 执行，不能阻塞；帧指针不能留到回调返回后使用。 */
typedef void (*can_rx_callback_t)(
    const st_can_frame_t *p_frame, en_core_can_fifo_t fifo);
typedef void (*can_tx_callback_t)(uint32_t buffer_mask);
typedef void (*can_error_callback_t)(uint32_t error_code);
//******************************* Typedefs end *******************************//

//************************ Function Prototypes begin *************************//
/**
 * @brief 检查 CubeMX 初始化状态，启动 CAN 并仅新增错误类通知。
 * @return CAN 操作状态。
 * Steps:
 *  1. 检查已配置的 HAL 句柄。
 *  2. Start 后激活错误通知，保留此前按需开启的 RX/TX 通知。
 * @note 过滤器与回调须在调用前完成；本函数不会清空回调。
 *       config_filter -> register_callback -> core_can_init -> 收发。
 */
en_core_can_status_t core_can_init(void);

/**
 * @brief 阻塞发送一帧，投递后轮询等待本帧 pending 清除或超时。
 * @return CAN 操作状态。
 * @note timeout_ms 为毫秒，0 表示无限等待；禁止在 ISR 调用。
 *       mailbox 全满立即返回 BUSY；超时请求中止本次 mailbox。
 *       同一实例由调用方串行访问，勿混用同步等待和回调续发。
 */
en_core_can_status_t core_can_transmit_frame(
    en_core_can_instance_t instance,
    const st_can_frame_t *p_frame,
    uint32_t timeout_ms);

/**
 * @brief 异步投递一帧，成功仅表示已装入 mailbox。
 * @return CAN 操作状态。
 */
en_core_can_status_t core_can_transmit_frame_async(
    en_core_can_instance_t instance,
    const st_can_frame_t *p_frame);

/**
 * @brief 轮询指定 FIFO 并取出一帧。
 * @return CAN 操作状态。
 * @note timeout_ms 为毫秒，0 表示无限等待；禁止在 ISR 调用。
 *       轮询前须关闭该 FIFO 的 MsgPending 通知，否则 ISR 会抢帧。
 *       默认未注册 RX 回调的 FIFO 关闭；或先调用 notification_disable。
 */
en_core_can_status_t core_can_receive_frame_sync(
    en_core_can_instance_t instance,
    st_can_frame_t *p_frame,
    en_core_can_fifo_t fifo,
    uint32_t timeout_ms);

/**
 * @brief 配置 32 位 ID 掩码过滤器，ID 和 mask 均右对齐。
 * @return CAN 操作状态。
 * @note 必须在 Start 前配置。mask 位为 1 表示必须匹配。
 *       FDCAN 标准/扩展过滤器各自从 0 编号，数量由 CubeMX 配置。
 *       bxCAN 共享 bank 分界固定为 14；封装自动转换寄存器布局。
 */
en_core_can_status_t core_can_config_filter(
    en_core_can_instance_t instance,
    uint8_t filter_bank,
    uint32_t id,
    uint32_t mask,
    uint8_t is_ext,
    en_core_can_fifo_t target_fifo);

/**
 * @brief 开启指定 FIFO 的消息挂起通知。
 * @return CAN 操作状态。
 */
en_core_can_status_t core_can_rx_notification_enable(
    en_core_can_instance_t instance,
    en_core_can_fifo_t fifo);

/**
 * @brief 关闭指定 FIFO 的消息挂起通知。
 * @return CAN 操作状态。
 */
en_core_can_status_t core_can_rx_notification_disable(
    en_core_can_instance_t instance,
    en_core_can_fifo_t fifo);

/**
 * @brief 注册 RX 回调，NULL 表示注销。
 * @return CAN 操作状态。
 * @note 自动开关对应 FIFO 通知；RX 帧指针仅在回调期间有效。
 */
en_core_can_status_t core_can_register_rx_callback(
    en_core_can_instance_t instance,
    en_core_can_fifo_t fifo,
    can_rx_callback_t pf_callback);

/**
 * @brief 注册 TX 回调，NULL 表示注销。
 * @return CAN 操作状态。
 * @note 自动开关 TX 完成通知；回调参数为缓冲区完成位掩码。
 */
en_core_can_status_t core_can_register_tx_callback(
    en_core_can_instance_t instance,
    can_tx_callback_t pf_callback);

/**
 * @brief 注册 ERROR 回调，NULL 表示注销。
 * @return CAN 操作状态。
 * @note 回调接收 HAL 错误码或 FDCAN 状态变化位；均在 ISR 执行。
 */
en_core_can_status_t core_can_register_error_callback(
    en_core_can_instance_t instance,
    can_error_callback_t pf_callback);

/** @brief 获取 HAL 句柄；非法或未配置实例返回 NULL。 */
st_can_handle_t *core_can_get_handle(en_core_can_instance_t instance);

/**
 * @brief 读取 FIFO 已有帧并调用 RX 回调，仅供 App ISR 转发。
 * @return CAN 操作状态。
 */
en_core_can_status_t core_can_irq_rx_pending(
    st_can_handle_t *p_handle,
    en_core_can_fifo_t fifo);

/**
 * @brief 分发 TX 完成事件，仅供 App ISR 转发。
 * @return CAN 操作状态。
 */
en_core_can_status_t core_can_irq_tx_complete(
    st_can_handle_t *p_handle,
    uint32_t buffer_mask);

/**
 * @brief 读取并转发 HAL 错误码，仅供 App ISR 转发。
 * @return CAN 操作状态。
 */
en_core_can_status_t core_can_irq_error(
    st_can_handle_t *p_handle);

#if CORE_CAN_USE_FDCAN
/**
 * @brief 转发 FDCAN 错误状态变化，仅供 App ISR 调用。
 * @return CAN 操作状态。
 * @note 回调参数为 FDCAN_IT_* 位，区别于 core_can_irq_error 的 HAL 错误码。
 */
en_core_can_status_t core_can_irq_error_status(
    st_can_handle_t *p_handle, uint32_t status_flags);
#endif
//************************* Function Prototypes end **************************//

#endif /* CAN_PORT_H */

//******************************* End of File ********************************//

