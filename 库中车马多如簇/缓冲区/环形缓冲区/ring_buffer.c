/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file ring_buffer.c
 *
 * @par dependencies
 * - ring_buffer.h
 *
 * @author YA
 *
 * @brief 字节环形缓冲区实现，不依赖外设和操作系统。
 *
 * Processing flow:
 *
 * 检查参数 -> 拷贝数据 -> 更新游标与计数。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "ring_buffer.h"

/**
 * @brief 检查对象是否具有有效存储及游标。
 *
 * Steps:
 *  1. 检查对象指针与存储。
 *  2. 检查容量、游标和计数。
 *
 * @param[in] p_ring : 缓冲区对象。
 *
 * @return ring_buffer_status_t : 检查结果。
 *
 */
static ring_buffer_status_t ring_buffer_validate(
    const ring_buffer_t * const p_ring)
{
    //-----------------------------参数校验begin------------------------------//
    if ((NULL == p_ring) || (NULL == p_ring->p_storage) ||
        (0U == p_ring->sz_capacity) ||
        (p_ring->sz_head >= p_ring->sz_capacity) ||
        (p_ring->sz_tail >= p_ring->sz_capacity) ||
        (p_ring->sz_count > p_ring->sz_capacity))
    {
        return RING_BUFFER_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    return RING_BUFFER_OK;
}

/**
 * @brief 绑定调用者的存储并初始化环形游标。
 *
 * Steps:
 *  1. 检查对象、存储和容量。
 *  2. 绑定存储并清空游标及计数。
 *
 * @param[out] p_ring     : 缓冲区对象。
 * @param[in] p_storage   : 独占字节存储。
 * @param[in] sz_capacity : 存储容量。
 *
 * @return ring_buffer_status_t : 初始化结果。
 *
 */
ring_buffer_status_t ring_buffer_init(
    ring_buffer_t * const p_ring,
    uint8_t * const       p_storage,
    size_t                sz_capacity)
{
    //-----------------------------参数校验begin------------------------------//
    if ((NULL == p_ring) || (NULL == p_storage) || (0U == sz_capacity))
    {
        return RING_BUFFER_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    // 绑定外部存储并初始化读写状态。
    p_ring->p_storage = p_storage;
    p_ring->sz_capacity = sz_capacity;
    p_ring->sz_head = 0U;
    p_ring->sz_tail = 0U;
    p_ring->sz_count = 0U;
    return RING_BUFFER_OK;
}

/**
 * @brief 检查剩余容量后整段复制，不覆盖未读字节。
 *
 * Steps:
 *  1. 检查参数及剩余容量。
 *  2. 整段复制字节并更新写游标和计数。
 *
 * @param[in,out] p_ring : 缓冲区对象。
 * @param[in] p_data     : 源数据。
 * @param[in] sz_length  : 源数据长度。
 *
 * @return ring_buffer_status_t : 写入结果。
 *
 */
ring_buffer_status_t ring_buffer_write(
    ring_buffer_t * const p_ring,
    const uint8_t * const p_data,
    size_t                sz_length)
{
    size_t sz_index;

    //-----------------------------参数校验begin------------------------------//
    if ((NULL == p_data) || (0U == sz_length) ||
        (RING_BUFFER_OK != ring_buffer_validate(p_ring)))
    {
        return RING_BUFFER_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    // 空间不足时整段拒绝写入，保留未读数据。
    if (sz_length > (p_ring->sz_capacity - p_ring->sz_count))
    {
        return RING_BUFFER_FULL;
    }
    // 逐字节复制并推进游标，到达末尾后回绕。
    for (sz_index = 0U; sz_index < sz_length; ++sz_index)
    {
        p_ring->p_storage[p_ring->sz_head] = p_data[sz_index];
        ++p_ring->sz_head;
        if (p_ring->sz_capacity == p_ring->sz_head)
        {
            p_ring->sz_head = 0U;
        }
    }
    // 累计新增的未读字节数。
    p_ring->sz_count += sz_length;
    return RING_BUFFER_OK;
}

/**
 * @brief 取出一段可用字节并更新读游标。
 *
 * Steps:
 *  1. 检查参数并确认是否有数据。
 *  2. 取出可用字节并更新读游标和计数。
 *
 * @param[in,out] p_ring  : 缓冲区对象。
 * @param[out] p_data     : 接收存储。
 * @param[in] sz_capacity : 接收存储容量。
 * @param[out] p_length   : 实际读取字节数。
 *
 * @return ring_buffer_status_t : 读取结果。
 *
 */
ring_buffer_status_t ring_buffer_read(
    ring_buffer_t * const p_ring,
    uint8_t * const       p_data,
    size_t                sz_capacity,
    size_t * const        p_length)
{
    size_t sz_length;
    size_t sz_index;

    //-----------------------------参数校验begin------------------------------//
    if ((NULL == p_data) || (NULL == p_length) || (0U == sz_capacity) ||
        (RING_BUFFER_OK != ring_buffer_validate(p_ring)))
    {
        return RING_BUFFER_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    // 默认读取长度为零，缓冲为空时直接返回。
    *p_length = 0U;
    if (0U == p_ring->sz_count)
    {
        return RING_BUFFER_EMPTY;
    }
    // 读取量不超过输出容量和当前未读字节数。
    sz_length = (sz_capacity < p_ring->sz_count)
        ? sz_capacity : p_ring->sz_count;
    // 逐字节复制并推进游标，到达末尾后回绕。
    for (sz_index = 0U; sz_index < sz_length; ++sz_index)
    {
        p_data[sz_index] = p_ring->p_storage[p_ring->sz_tail];
        ++p_ring->sz_tail;
        if (p_ring->sz_capacity == p_ring->sz_tail)
        {
            p_ring->sz_tail = 0U;
        }
    }
    // 扣除已读字节，并输出实际读取长度。
    p_ring->sz_count -= sz_length;
    *p_length = sz_length;
    return RING_BUFFER_OK;
}

/**
 * @brief 丢弃未读数据并复位两个游标。
 *
 * Steps:
 *  1. 检查缓冲对象。
 *  2. 清空读写游标和未读计数。
 *
 * @param[in,out] p_ring : 缓冲区对象。
 *
 * @return ring_buffer_status_t : 复位结果。
 *
 */
ring_buffer_status_t ring_buffer_reset(
    ring_buffer_t * const p_ring)
{
    //-----------------------------参数校验begin------------------------------//
    if (RING_BUFFER_OK != ring_buffer_validate(p_ring))
    {
        return RING_BUFFER_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    // 丢弃未读数据并复位读写游标。
    p_ring->sz_head = 0U;
    p_ring->sz_tail = 0U;
    p_ring->sz_count = 0U;
    return RING_BUFFER_OK;
}
