/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file can_port.c
 *
 * @par dependencies
 * - can_port.h
 * - can_port_config.h
 * - stddef.h
 *
 * @author YA
 *
 * @brief 经典 CAN / CAN FD 多实例收发、过滤器及回调分发。
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

//****************************** Includes begin ******************************//
#include "can_port.h"
#include "can_port_config.h"
#include <stddef.h>
//******************************* Includes end *******************************//

//************************** Static Variables begin **************************//
static can_rx_callback_t g_pf_can_rx_callbacks[CORE_CAN_MAX][2] =
{
    {NULL}
};
static can_tx_callback_t g_pf_can_tx_callbacks[CORE_CAN_MAX] = {NULL};
static can_error_callback_t g_pf_can_error_callbacks[CORE_CAN_MAX] = {NULL};
//*************************** Static Variables end ***************************//

//****************************** Typedefs begin ******************************//
#if CORE_CAN_USE_FDCAN
typedef FDCAN_TxHeaderTypeDef st_can_tx_header_t;
typedef FDCAN_RxHeaderTypeDef st_can_rx_header_t;
#else
typedef CAN_TxHeaderTypeDef st_can_tx_header_t;
typedef CAN_RxHeaderTypeDef st_can_rx_header_t;
#endif
//******************************* Typedefs end *******************************//

//****************************** Defines begin *******************************//
#if CORE_CAN_USE_FDCAN
#define CORE_CAN_HAL_GET_STATE       HAL_FDCAN_GetState
#define CORE_CAN_HAL_STATE_RESET     HAL_FDCAN_STATE_RESET
#define CORE_CAN_HAL_GET_RX          HAL_FDCAN_GetRxMessage
#define CORE_CAN_HAL_RX_FILL         HAL_FDCAN_GetRxFifoFillLevel
#define CORE_CAN_HAL_TX_PENDING      HAL_FDCAN_IsTxBufferMessagePending
#define CORE_CAN_HAL_ABORT_TX        HAL_FDCAN_AbortTxRequest
#define CORE_CAN_IT_RX0              FDCAN_IT_RX_FIFO0_NEW_MESSAGE
#define CORE_CAN_IT_RX1              FDCAN_IT_RX_FIFO1_NEW_MESSAGE
#define CORE_CAN_IT_TX               FDCAN_IT_TX_COMPLETE
#if defined(FDCAN_TX_BUFFER31)
#define CORE_CAN_TX_BUFFER_MASK      (0xFFFFFFFFUL)
#else
#define CORE_CAN_TX_BUFFER_MASK      \
    (FDCAN_TX_BUFFER0 | FDCAN_TX_BUFFER1 | FDCAN_TX_BUFFER2)
#endif
#else
#define CORE_CAN_HAL_GET_STATE       HAL_CAN_GetState
#define CORE_CAN_HAL_STATE_RESET     HAL_CAN_STATE_RESET
#define CORE_CAN_HAL_GET_RX          HAL_CAN_GetRxMessage
#define CORE_CAN_HAL_RX_FILL         HAL_CAN_GetRxFifoFillLevel
#define CORE_CAN_HAL_TX_PENDING      HAL_CAN_IsTxMessagePending
#define CORE_CAN_HAL_ABORT_TX        HAL_CAN_AbortTxRequest
#define CORE_CAN_IT_RX0              CAN_IT_RX_FIFO0_MSG_PENDING
#define CORE_CAN_IT_RX1              CAN_IT_RX_FIFO1_MSG_PENDING
#define CORE_CAN_IT_TX               CAN_IT_TX_MAILBOX_EMPTY
#define CORE_CAN_TX_BUFFER_MASK      (0x7U)
#endif
//******************************* Defines end ********************************//

//***************************** Constants begin ******************************//
#if CORE_CAN_USE_FDCAN
/* 使用 HAL 命名常量，兼容不同版本的 DLC 位移约定。 */
static const uint8_t g_can_fd_lengths[16] =
{
    0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U,
    8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U
};
static const uint32_t g_can_fd_dlc[16] =
{
    FDCAN_DLC_BYTES_0, FDCAN_DLC_BYTES_1,
    FDCAN_DLC_BYTES_2, FDCAN_DLC_BYTES_3,
    FDCAN_DLC_BYTES_4, FDCAN_DLC_BYTES_5,
    FDCAN_DLC_BYTES_6, FDCAN_DLC_BYTES_7,
    FDCAN_DLC_BYTES_8, FDCAN_DLC_BYTES_12,
    FDCAN_DLC_BYTES_16, FDCAN_DLC_BYTES_20,
    FDCAN_DLC_BYTES_24, FDCAN_DLC_BYTES_32,
    FDCAN_DLC_BYTES_48, FDCAN_DLC_BYTES_64
};

#endif
//****************************** Constants end *******************************//

//************************** Static Functions begin **************************//
/**
 * @brief 映射 HAL 操作状态。
 * @return CAN 操作状态或转换结果。
 */
static en_core_can_status_t core_can_from_hal(HAL_StatusTypeDef status)
{
    switch (status)
    {
        case HAL_OK:
            return CORE_CAN_OK;
        case HAL_BUSY:
            return CORE_CAN_ERR_BUSY;
        case HAL_TIMEOUT:
            return CORE_CAN_ERR_TIMEOUT;
        default:
            return CORE_CAN_ERROR;
    }
}

#if CORE_CAN_USE_FDCAN
/**
 * @brief 将真实字节数转换为 HAL DLC；不自动补齐非法长度。
 * @return CAN 操作状态。
 */
static en_core_can_status_t core_can_length_to_dlc(
    uint8_t length, uint32_t *p_dlc)
{
    uint32_t i;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_dlc)
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    for (i = 0U; i < 16U; i++)
    {
        if (length == g_can_fd_lengths[i])
        {
            *p_dlc = g_can_fd_dlc[i];
            return CORE_CAN_OK;
        }
    }
    return CORE_CAN_ERR_ARGUMENT;
}

/**
 * @brief 将接收的 HAL DLC 转换为真实字节数。
 * @return CAN 操作状态。
 */
static en_core_can_status_t core_can_dlc_to_length(
    uint32_t dlc, uint8_t *p_length)
{
    uint32_t i;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_length)
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    for (i = 0U; i < 16U; i++)
    {
        if (dlc == g_can_fd_dlc[i])
        {
            *p_length = g_can_fd_lengths[i];
            return CORE_CAN_OK;
        }
    }
    return CORE_CAN_ERROR;
}
#endif

/**
 * @brief 激活通知，FDCAN 的 TX 通知覆盖 HAL 支持的缓冲区位。
 * @return CAN 操作状态。
 */
static HAL_StatusTypeDef core_can_activate_notification(
    st_can_handle_t *p_handle, uint32_t notification)
{
#if CORE_CAN_USE_FDCAN
    return HAL_FDCAN_ActivateNotification(
        p_handle, notification, CORE_CAN_TX_BUFFER_MASK);
#else
    return HAL_CAN_ActivateNotification(p_handle, notification);
#endif
}

/**
 * @brief 关闭所选通知。
 * @return CAN 操作状态。
 */
static HAL_StatusTypeDef core_can_deactivate_notification(
    st_can_handle_t *p_handle, uint32_t notification)
{
#if CORE_CAN_USE_FDCAN
    return HAL_FDCAN_DeactivateNotification(p_handle, notification);
#else
    return HAL_CAN_DeactivateNotification(p_handle, notification);
#endif
}

/**
 * @brief 启动句柄并开启错误通知，不清空预先注册的 RX/TX 通知。
 * @return CAN 操作状态。
 */
static en_core_can_status_t core_can_start_handle(
    st_can_handle_t *p_handle)
{
    HAL_StatusTypeDef hal_status;
    uint32_t notification;

#if CORE_CAN_USE_FDCAN
    /* FDCAN 默认可能放行未匹配报文；显式拒绝后才遵循过滤器规则。 */
    hal_status = HAL_FDCAN_ConfigGlobalFilter(
        p_handle, FDCAN_REJECT, FDCAN_REJECT,
        FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
    if (HAL_OK != hal_status)
    {
        return core_can_from_hal(hal_status);
    }
    hal_status = HAL_FDCAN_Start(p_handle);
    notification = FDCAN_IT_BUS_OFF | FDCAN_IT_ERROR_WARNING |
        FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_ARB_PROTOCOL_ERROR |
        FDCAN_IT_DATA_PROTOCOL_ERROR;
#else
    hal_status = HAL_CAN_Start(p_handle);
    notification = CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_LAST_ERROR_CODE;
#endif
    if (HAL_OK != hal_status)
    {
        return core_can_from_hal(hal_status);
    }
    return core_can_from_hal(
        core_can_activate_notification(p_handle, notification));
}

/**
 * @brief 检查句柄已由 CubeMX 初始化。
 * @return CAN 操作状态或转换结果。
 */
static en_core_can_status_t core_can_validate_handle(
    st_can_handle_t *p_handle)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_handle) ||
        (CORE_CAN_HAL_STATE_RESET == CORE_CAN_HAL_GET_STATE(p_handle)))
    {
        return CORE_CAN_ERR_NOT_READY;
    }
    //--------------------------参数检查 end--------------------------//

    return CORE_CAN_OK;
}

/**
 * @brief 将已验证的 FIFO 枚举映射为 HAL FIFO。
 * @return CAN 操作状态或转换结果。
 */
static uint32_t core_can_fifo_to_hal(en_core_can_fifo_t fifo)
{
#if CORE_CAN_USE_FDCAN
    return (CORE_CAN_FIFO0 == fifo) ? FDCAN_RX_FIFO0 : FDCAN_RX_FIFO1;
#else
    return (CORE_CAN_FIFO0 == fifo) ? CAN_RX_FIFO0 : CAN_RX_FIFO1;
#endif
}

/**
 * @brief 转换已验证的发送帧为 HAL 头。
 * @return CAN 操作状态或转换结果。
 */
static en_core_can_status_t core_can_frame_to_tx_header(
    const st_can_frame_t *p_frame,
    st_can_tx_header_t *p_header)
{
    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_frame) || (NULL == p_header))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

#if CORE_CAN_USE_FDCAN
    p_header->Identifier = p_frame->id;
    p_header->IdType = (0U != p_frame->is_ext)
        ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    p_header->TxFrameType = (0U != p_frame->is_rtr)
        ? FDCAN_REMOTE_FRAME : FDCAN_DATA_FRAME;
    p_header->FDFormat = (0U != p_frame->is_fd)
        ? FDCAN_FD_CAN : FDCAN_CLASSIC_CAN;
    p_header->BitRateSwitch = (0U != p_frame->is_brs)
        ? FDCAN_BRS_ON : FDCAN_BRS_OFF;
    p_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    p_header->TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    p_header->MessageMarker = 0U;
    return core_can_length_to_dlc(p_frame->dlc, &p_header->DataLength);
#else
    p_header->StdId = (0U == p_frame->is_ext) ? p_frame->id : 0U;
    p_header->ExtId = (0U != p_frame->is_ext) ? p_frame->id : 0U;
    p_header->IDE = (0U != p_frame->is_ext) ? CAN_ID_EXT : CAN_ID_STD;
    p_header->RTR = (0U != p_frame->is_rtr) ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    p_header->DLC = p_frame->dlc;
    p_header->TransmitGlobalTime = DISABLE;
    return CORE_CAN_OK;
#endif
}

/**
 * @brief 转换接收头与数据；遥控帧不返回未定义载荷。
 * @return CAN 操作状态或转换结果。
 */
static en_core_can_status_t core_can_rx_header_to_frame(
    const st_can_rx_header_t *p_header,
    const uint8_t *p_data,
    st_can_frame_t *p_frame)
{
    uint32_t i;
    st_can_frame_t frame = {0};
#if CORE_CAN_USE_FDCAN
    en_core_can_status_t status;
#endif

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_header) || (NULL == p_data) || (NULL == p_frame))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

#if CORE_CAN_USE_FDCAN
    status = core_can_dlc_to_length(p_header->DataLength, &frame.dlc);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    frame.id = p_header->Identifier;
    frame.is_ext = (FDCAN_EXTENDED_ID == p_header->IdType) ? 1U : 0U;
    frame.is_rtr = (FDCAN_REMOTE_FRAME == p_header->RxFrameType) ? 1U : 0U;
    frame.is_fd = (FDCAN_FD_CAN == p_header->FDFormat) ? 1U : 0U;
    frame.is_brs = (FDCAN_BRS_ON == p_header->BitRateSwitch) ? 1U : 0U;
    if ((0U == frame.is_fd) && (8U < frame.dlc))
    {
        return CORE_CAN_ERROR;
    }
#else
    if (8U < p_header->DLC)
    {
        return CORE_CAN_ERROR;
    }
    frame.is_ext = (CAN_ID_EXT == p_header->IDE) ? 1U : 0U;
    frame.is_rtr = (CAN_RTR_REMOTE == p_header->RTR) ? 1U : 0U;
    frame.id = (0U != frame.is_ext) ? p_header->ExtId : p_header->StdId;
    frame.dlc = (uint8_t)p_header->DLC;
#endif
    if (0U == frame.is_rtr)
    {
        for (i = 0U; i < frame.dlc; i++)
        {
            frame.data[i] = p_data[i];
        }
    }
    *p_frame = frame;
    return CORE_CAN_OK;
}

/**
 * @brief 共用异步投递路径，并返回本帧的 发送缓冲区 位掩码。
 * @return CAN 操作状态或转换结果。
 */
static en_core_can_status_t core_can_submit_frame(
    st_can_handle_t *p_handle,
    const st_can_frame_t *p_frame,
    uint32_t *p_mailbox)
{
    st_can_tx_header_t header = {0};
    en_core_can_status_t status;
#if CORE_CAN_USE_FDCAN
    uint32_t primask;
    HAL_StatusTypeDef hal_status;
#endif

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_mailbox)
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    status = core_can_frame_to_tx_header(p_frame, &header);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

#if CORE_CAN_USE_FDCAN
    /* 将投递和读取本次 buffer mask 作为不可分割的短操作。 */
    primask = __get_PRIMASK();
    __disable_irq();
    if (0U == HAL_FDCAN_GetTxFifoFreeLevel(p_handle))
    {
        __set_PRIMASK(primask);
        return CORE_CAN_ERR_BUSY;
    }
    hal_status = HAL_FDCAN_AddMessageToTxFifoQ(
        p_handle, &header, (uint8_t *)p_frame->data);
    if (HAL_OK == hal_status)
    {
        *p_mailbox = HAL_FDCAN_GetLatestTxFifoQRequestBuffer(p_handle);
    }
    __set_PRIMASK(primask);
    return core_can_from_hal(hal_status);
#else
    if (0U == HAL_CAN_GetTxMailboxesFreeLevel(p_handle))
    {
        return CORE_CAN_ERR_BUSY;
    }
    return core_can_from_hal(HAL_CAN_AddTxMessage(
        p_handle, &header, (uint8_t *)p_frame->data, p_mailbox));
#endif
}

/**
 * @brief 检查实例范围、配置与 CubeMX 初始化状态。
 * @return CAN 操作状态或转换结果。
 */
static en_core_can_status_t core_can_validate_instance(
    en_core_can_instance_t instance)
{
    //-------------------------参数检查 begin-------------------------//
    if ((uint32_t)CORE_CAN_MAX <= (uint32_t)instance)
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//

    return core_can_validate_handle(g_can_configs[instance].p_handle);
}

/**
 * @brief 按 HAL 句柄查找实例；失败返回 CORE_CAN_MAX。
 * @return CAN 操作状态或转换结果。
 */
static en_core_can_instance_t core_can_instance_from_handle(
    st_can_handle_t *p_handle)
{
    uint32_t i;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return CORE_CAN_MAX;
    }
    //--------------------------参数检查 end--------------------------//

    for (i = 0U; i < (uint32_t)CORE_CAN_MAX; i++)
    {
        if (g_can_configs[i].p_handle == p_handle)
        {
            return (en_core_can_instance_t)i;
        }
    }
    return CORE_CAN_MAX;
}

/**
 * @brief 校验帧标志、长度、ID 与硬件配置。
 * @return CAN 操作状态或转换结果。
 */
static en_core_can_status_t core_can_validate_frame(
    en_core_can_instance_t instance,
    const st_can_frame_t *p_frame)
{
    en_core_can_status_t status;
#if CORE_CAN_USE_FDCAN
    uint32_t dlc;
#endif

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_frame) || (CORE_CAN_DATA_MAX < p_frame->dlc) ||
        (1U < p_frame->is_ext) || (1U < p_frame->is_rtr) ||
        (1U < p_frame->is_fd) || (1U < p_frame->is_brs))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    if (((0U == p_frame->is_ext) && (0x7FFU < p_frame->id)) ||
        ((0U != p_frame->is_ext) && (0x1FFFFFFFU < p_frame->id)) ||
        ((0U == p_frame->is_fd) &&
         ((8U < p_frame->dlc) || (0U != p_frame->is_brs))) ||
        ((0U != p_frame->is_fd) && (0U != p_frame->is_rtr)))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    status = core_can_validate_instance(instance);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
#if CORE_CAN_USE_FDCAN
    status = core_can_length_to_dlc(p_frame->dlc, &dlc);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    if (((0U != p_frame->is_fd) &&
         (FDCAN_FRAME_CLASSIC ==
          g_can_configs[instance].p_handle->Init.FrameFormat)) ||
        ((0U != p_frame->is_brs) &&
         (FDCAN_FRAME_FD_BRS !=
          g_can_configs[instance].p_handle->Init.FrameFormat)))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
#else
    if ((0U != p_frame->is_fd) || (0U != p_frame->is_brs))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
#endif
    //--------------------------参数检查 end--------------------------//

    return CORE_CAN_OK;
}

//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 启动 CAN 并新增错误通知，保留预先配置的回调和通知。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_init(void)
{
    uint32_t i;
    st_can_handle_t *p_handle;
    en_core_can_status_t status;
    uint8_t configured = 0U;

    //-------------------------参数检查 begin-------------------------//
    for (i = 0U; i < (uint32_t)CORE_CAN_MAX; i++)
    {
        p_handle = g_can_configs[i].p_handle;
        if (NULL != p_handle)
        {
            status = core_can_validate_handle(p_handle);
            if (CORE_CAN_OK != status)
            {
                return status;
            }
            configured = 1U;
        }
    }
    if (0U == configured)
    {
        return CORE_CAN_ERR_NOT_READY;
    }
    //--------------------------参数检查 end--------------------------//

    for (i = 0U; i < (uint32_t)CORE_CAN_MAX; i++)
    {
        p_handle = g_can_configs[i].p_handle;
        if (NULL == p_handle)
        {
            continue;
        }
        status = core_can_start_handle(p_handle);
        if (CORE_CAN_OK != status)
        {
            return status;
        }
    }
    return CORE_CAN_OK;
}

/**
 * @brief 异步投递一帧，立即返回。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_transmit_frame_async(
    en_core_can_instance_t instance,
    const st_can_frame_t *p_frame)
{
    uint32_t mailbox;
    en_core_can_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_can_validate_frame(instance, p_frame);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    return core_can_submit_frame(
        g_can_configs[instance].p_handle, p_frame, &mailbox);
}

/**
 * @brief 阻塞发送一帧，投递后轮询等待本帧 pending 清除或超时。
 * @note timeout_ms 为 0 时无限等待；禁止在 ISR 中调用。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_transmit_frame(
    en_core_can_instance_t instance,
    const st_can_frame_t *p_frame,
    uint32_t timeout_ms)
{
    uint32_t mailbox;
    uint32_t start;
    en_core_can_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_can_validate_frame(instance, p_frame);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    status = core_can_submit_frame(
        g_can_configs[instance].p_handle, p_frame, &mailbox);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    start = HAL_GetTick();
    while (0U != CORE_CAN_HAL_TX_PENDING(
        g_can_configs[instance].p_handle, mailbox))
    {
        if ((0U != timeout_ms) &&
            ((HAL_GetTick() - start) >= timeout_ms))
        {
            (void)CORE_CAN_HAL_ABORT_TX(
                g_can_configs[instance].p_handle, mailbox);
            return CORE_CAN_ERR_TIMEOUT;
        }
    }
    return CORE_CAN_OK;
}

/**
 * @brief 轮询指定 FIFO 并读取一帧，调用前须关闭该 FIFO 通知。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_receive_frame_sync(
    en_core_can_instance_t instance,
    st_can_frame_t *p_frame,
    en_core_can_fifo_t fifo,
    uint32_t timeout_ms)
{
    uint32_t start;
    st_can_rx_header_t header = {0};
    uint8_t data[CORE_CAN_DATA_MAX] = {0};
    en_core_can_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_frame)
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    //--------------------------参数检查 end--------------------------//
    //-------------------------参数检查 begin-------------------------//
    if ((CORE_CAN_FIFO0 != fifo) && (CORE_CAN_FIFO1 != fifo))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    status = core_can_validate_instance(instance);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    start = HAL_GetTick();
    while (0U == CORE_CAN_HAL_RX_FILL(
        g_can_configs[instance].p_handle, core_can_fifo_to_hal(fifo)))
    {
        if ((0U != timeout_ms) &&
            ((HAL_GetTick() - start) >= timeout_ms))
        {
            return CORE_CAN_ERR_TIMEOUT;
        }
    }
    status = core_can_from_hal(CORE_CAN_HAL_GET_RX(
        g_can_configs[instance].p_handle, core_can_fifo_to_hal(fifo),
        &header, data));
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    return core_can_rx_header_to_frame(&header, data, p_frame);
}

/**
 * @brief 配置 ID 掩码过滤器，强制匹配 IDE，忽略 RTR。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_config_filter(
    en_core_can_instance_t instance,
    uint8_t filter_bank,
    uint32_t id,
    uint32_t mask,
    uint8_t is_ext,
    en_core_can_fifo_t target_fifo)
{
    en_core_can_status_t status;
#if CORE_CAN_USE_FDCAN
    FDCAN_FilterTypeDef filter = {0};
    uint32_t filter_count;
#else
    CAN_FilterTypeDef filter = {0};
    uint32_t reg_id;
    uint32_t reg_mask;
#endif

    //-------------------------参数检查 begin-------------------------//
    if ((1U < is_ext) ||
        ((CORE_CAN_FIFO0 != target_fifo) && (CORE_CAN_FIFO1 != target_fifo)))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    if (((0U == is_ext) && ((0x7FFU < id) || (0x7FFU < mask))) ||
        ((0U != is_ext) &&
         ((0x1FFFFFFFU < id) || (0x1FFFFFFFU < mask))))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    status = core_can_validate_instance(instance);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

#if CORE_CAN_USE_FDCAN
    filter_count = (0U != is_ext)
        ? (g_can_configs[instance].p_handle)->Init.ExtFiltersNbr
        : (g_can_configs[instance].p_handle)->Init.StdFiltersNbr;
    if (filter_count <= filter_bank)
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    filter.IdType = (0U != is_ext) ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    filter.FilterIndex = filter_bank;
    filter.FilterType = FDCAN_FILTER_MASK;
    filter.FilterConfig = (CORE_CAN_FIFO0 == target_fifo)
        ? FDCAN_FILTER_TO_RXFIFO0 : FDCAN_FILTER_TO_RXFIFO1;
    /* HAL FDCAN 接收右对齐 ID；不能套用 bxCAN 的寄存器左移规则。 */
    filter.FilterID1 = id;
    filter.FilterID2 = mask;
    return core_can_from_hal(HAL_FDCAN_ConfigFilter(
        g_can_configs[instance].p_handle, &filter));
#else
#if defined(CAN2)
    if ((28U <= filter_bank) ||
        ((CAN2 == g_can_configs[instance].p_handle->Instance) &&
         (14U > filter_bank)) ||
        ((CAN2 != g_can_configs[instance].p_handle->Instance) &&
         (14U <= filter_bank)))
#else
    if (14U <= filter_bank)
#endif
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    filter.FilterBank = filter_bank;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterFIFOAssignment = (CORE_CAN_FIFO0 == target_fifo)
        ? CAN_FILTER_FIFO0 : CAN_FILTER_FIFO1;
    filter.FilterActivation = CAN_FILTER_ENABLE;
    filter.SlaveStartFilterBank = 14U;
    if (0U != is_ext)
    {
        reg_id = (id << 3U) | CAN_ID_EXT;
        reg_mask = (mask << 3U) | CAN_ID_EXT;
        filter.FilterIdHigh = (reg_id >> 16U) & 0xFFFFU;
        filter.FilterIdLow = reg_id & 0xFFFFU;
        filter.FilterMaskIdHigh = (reg_mask >> 16U) & 0xFFFFU;
        filter.FilterMaskIdLow = reg_mask & 0xFFFFU;
    }
    else
    {
        filter.FilterIdHigh = (id & 0x7FFU) << 5U;
        filter.FilterIdLow = 0U;
        filter.FilterMaskIdHigh = (mask & 0x7FFU) << 5U;
        filter.FilterMaskIdLow = CAN_ID_EXT;
    }
    return core_can_from_hal(HAL_CAN_ConfigFilter(
        g_can_configs[instance].p_handle, &filter));
#endif
}

/**
 * @brief 开启指定 FIFO 的消息挂起通知。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_rx_notification_enable(
    en_core_can_instance_t instance,
    en_core_can_fifo_t fifo)
{
    uint32_t notification;
    en_core_can_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((CORE_CAN_FIFO0 != fifo) && (CORE_CAN_FIFO1 != fifo))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    status = core_can_validate_instance(instance);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    notification = (CORE_CAN_FIFO0 == fifo)
        ? CORE_CAN_IT_RX0 : CORE_CAN_IT_RX1;
    return core_can_from_hal(core_can_activate_notification(
        g_can_configs[instance].p_handle, notification));
}

/**
 * @brief 关闭指定 FIFO 的消息挂起通知。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_rx_notification_disable(
    en_core_can_instance_t instance,
    en_core_can_fifo_t fifo)
{
    uint32_t notification;
    en_core_can_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((CORE_CAN_FIFO0 != fifo) && (CORE_CAN_FIFO1 != fifo))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    status = core_can_validate_instance(instance);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    notification = (CORE_CAN_FIFO0 == fifo)
        ? CORE_CAN_IT_RX0 : CORE_CAN_IT_RX1;
    return core_can_from_hal(core_can_deactivate_notification(
        g_can_configs[instance].p_handle, notification));
}

/**
 * @brief 注册或注销对应 FIFO 的 RX 回调，并自动开关通知。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_register_rx_callback(
    en_core_can_instance_t instance,
    en_core_can_fifo_t fifo,
    can_rx_callback_t pf_callback)
{
    can_rx_callback_t pf_previous;
    en_core_can_status_t status;

    //-------------------------参数检查 begin-------------------------//
    if ((CORE_CAN_FIFO0 != fifo) && (CORE_CAN_FIFO1 != fifo))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    status = core_can_validate_instance(instance);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    pf_previous = g_pf_can_rx_callbacks[instance][fifo];
    if (NULL == pf_callback)
    {
        status = core_can_rx_notification_disable(instance, fifo);
        if (CORE_CAN_OK == status)
        {
            g_pf_can_rx_callbacks[instance][fifo] = NULL;
        }
        return status;
    }
    g_pf_can_rx_callbacks[instance][fifo] = pf_callback;
    status = core_can_rx_notification_enable(instance, fifo);
    if (CORE_CAN_OK != status)
    {
        g_pf_can_rx_callbacks[instance][fifo] = pf_previous;
    }
    return status;
}

/**
 * @brief 注册 TX 回调并按需开关 mailbox 完成通知。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_register_tx_callback(
    en_core_can_instance_t instance,
    can_tx_callback_t pf_callback)
{
    can_tx_callback_t pf_previous;
    en_core_can_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_can_validate_instance(instance);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    pf_previous = g_pf_can_tx_callbacks[instance];
    if (NULL == pf_callback)
    {
        status = core_can_from_hal(core_can_deactivate_notification(
            g_can_configs[instance].p_handle, CORE_CAN_IT_TX));
        if (CORE_CAN_OK == status)
        {
            g_pf_can_tx_callbacks[instance] = NULL;
        }
        return status;
    }
    g_pf_can_tx_callbacks[instance] = pf_callback;
    status = core_can_from_hal(core_can_activate_notification(
        g_can_configs[instance].p_handle, CORE_CAN_IT_TX));
    if (CORE_CAN_OK != status)
    {
        g_pf_can_tx_callbacks[instance] = pf_previous;
    }
    return status;
}

/**
 * @brief 注册错误回调，NULL 注销；错误通知仍由 init 开启。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_register_error_callback(
    en_core_can_instance_t instance,
    can_error_callback_t pf_callback)
{
    en_core_can_status_t status;

    //-------------------------参数检查 begin-------------------------//
    status = core_can_validate_instance(instance);
    if (CORE_CAN_OK != status)
    {
        return status;
    }
    //--------------------------参数检查 end--------------------------//

    g_pf_can_error_callbacks[instance] = pf_callback;
    return CORE_CAN_OK;
}

/**
 * @brief 获取实例句柄，非法或未配置时返回 NULL。
 * @return CAN 操作状态或转换结果。
 */
st_can_handle_t *core_can_get_handle(en_core_can_instance_t instance)
{
    //-------------------------参数检查 begin-------------------------//
    if ((uint32_t)CORE_CAN_MAX <= (uint32_t)instance)
    {
        return NULL;
    }
    //--------------------------参数检查 end--------------------------//

    return g_can_configs[instance].p_handle;
}

/**
 * @brief ISR 按 FIFO 快照逐帧读取并调用用户回调。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_irq_rx_pending(
    st_can_handle_t *p_handle,
    en_core_can_fifo_t fifo)
{
    en_core_can_instance_t instance;
    st_can_rx_header_t header = {0};
    uint8_t data[CORE_CAN_DATA_MAX] = {0};
    st_can_frame_t frame;
    en_core_can_status_t status;
    uint32_t remaining;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_handle) ||
        ((CORE_CAN_FIFO0 != fifo) && (CORE_CAN_FIFO1 != fifo)))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    instance = core_can_instance_from_handle(p_handle);
    if (CORE_CAN_MAX == instance)
    {
        return CORE_CAN_ERR_NOT_READY;
    }
    //--------------------------参数检查 end--------------------------//

    /* FDCAN 是新消息事件；按入口快照排空已有帧，避免遗留帧滞留。 */
    remaining = CORE_CAN_HAL_RX_FILL(p_handle, core_can_fifo_to_hal(fifo));
    while (0U != remaining)
    {
        status = core_can_from_hal(CORE_CAN_HAL_GET_RX(
            p_handle, core_can_fifo_to_hal(fifo), &header, data));
        if (CORE_CAN_OK != status)
        {
            return status;
        }
        status = core_can_rx_header_to_frame(&header, data, &frame);
        if (CORE_CAN_OK != status)
        {
            return status;
        }
        if (NULL != g_pf_can_rx_callbacks[instance][fifo])
        {
            g_pf_can_rx_callbacks[instance][fifo](&frame, fifo);
        }
        remaining--;
    }
    return CORE_CAN_OK;
}

/**
 * @brief 转发发送缓冲区完成位掩码。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_irq_tx_complete(
    st_can_handle_t *p_handle,
    uint32_t buffer_mask)
{
    en_core_can_instance_t instance;

    //-------------------------参数检查 begin-------------------------//
    if ((NULL == p_handle) || (0U == buffer_mask) ||
        (0U != (buffer_mask & ~CORE_CAN_TX_BUFFER_MASK)))
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    instance = core_can_instance_from_handle(p_handle);
    if (CORE_CAN_MAX == instance)
    {
        return CORE_CAN_ERR_NOT_READY;
    }
    //--------------------------参数检查 end--------------------------//

    if (NULL != g_pf_can_tx_callbacks[instance])
    {
        g_pf_can_tx_callbacks[instance](buffer_mask);
    }
    return CORE_CAN_OK;
}

/**
 * @brief 读取 HAL 错误位掩码并原样转发。
 * @return CAN 操作状态或转换结果。
 */
en_core_can_status_t core_can_irq_error(st_can_handle_t *p_handle)
{
    en_core_can_instance_t instance;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    instance = core_can_instance_from_handle(p_handle);
    if (CORE_CAN_MAX == instance)
    {
        return CORE_CAN_ERR_NOT_READY;
    }
    //--------------------------参数检查 end--------------------------//

    if (NULL != g_pf_can_error_callbacks[instance])
    {
#if CORE_CAN_USE_FDCAN
        g_pf_can_error_callbacks[instance](HAL_FDCAN_GetError(p_handle));
#else
        g_pf_can_error_callbacks[instance](HAL_CAN_GetError(p_handle));
#endif
    }
    return CORE_CAN_OK;
}

#if CORE_CAN_USE_FDCAN
/**
 * @brief 转发 FDCAN 错误状态变化位；保留与 HAL 错误码分开的来源。
 * @return CAN 操作状态。
 */
en_core_can_status_t core_can_irq_error_status(
    st_can_handle_t *p_handle, uint32_t status_flags)
{
    en_core_can_instance_t instance;

    //-------------------------参数检查 begin-------------------------//
    if (NULL == p_handle)
    {
        return CORE_CAN_ERR_ARGUMENT;
    }
    instance = core_can_instance_from_handle(p_handle);
    if (CORE_CAN_MAX == instance)
    {
        return CORE_CAN_ERR_NOT_READY;
    }
    //--------------------------参数检查 end--------------------------//

    if (NULL != g_pf_can_error_callbacks[instance])
    {
        /* ErrorStatus 的值为 FDCAN_IT_* 状态变化位，不是 GetError。 */
        g_pf_can_error_callbacks[instance](status_flags);
    }
    return CORE_CAN_OK;
}
#endif

//************************* Function Definitions end *************************//

//******************************* End of File ********************************//

