/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file ab_buffer.c
 *
 * @par dependencies
 * - ab_buffer.h
 *
 * @author YA
 *
 * @brief AB 双缓冲所有权交接，不依赖 HAL、OS 或堆内存。
 *
 * Processing flow:
 *
 * 空闲 -> 写入中 -> 就绪 -> 读取中 -> 空闲。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "ab_buffer.h"

/**
 * @brief 检查对象存储、索引及各块长度和状态。
 *
 * Steps:
 *  1. 检查对象、地址、容量及索引范围。
 *  2. 检查块状态和长度的一致性。
 *
 * @param[in] p_buffer : 双缓冲对象。
 *
 * @return ab_buffer_status_t : 检查结果。
 */
static ab_buffer_status_t ab_buffer_validate(
    const ab_buffer_t * const p_buffer)
{
    size_t sz_index;

    //-----------------------------参数校验begin------------------------------//
    if ((NULL == p_buffer) || (NULL == p_buffer->ap_storage[0]) ||
        (NULL == p_buffer->ap_storage[1]) ||
        (p_buffer->ap_storage[0] == p_buffer->ap_storage[1]) ||
        (0U == p_buffer->sz_capacity) ||
        (2U <= p_buffer->sz_write_index) ||
        (2U <= p_buffer->sz_read_index))
    {
        return AB_BUFFER_INVALID_PARAM;
    }
    for (sz_index = 0U; sz_index < 2U; ++sz_index)
    {
        switch (p_buffer->ae_state[sz_index])
        {
            case AB_BUFFER_FREE:
            case AB_BUFFER_WRITING:
                if (0U != p_buffer->asz_length[sz_index])
                {
                    return AB_BUFFER_INVALID_PARAM;
                }
                break;
            case AB_BUFFER_READY:
            case AB_BUFFER_READING:
                if ((0U == p_buffer->asz_length[sz_index]) ||
                    (p_buffer->asz_length[sz_index] >
                     p_buffer->sz_capacity))
                {
                    return AB_BUFFER_INVALID_PARAM;
                }
                break;
            default:
                return AB_BUFFER_INVALID_PARAM;
        }
    }
    //------------------------------参数校验end-------------------------------//
    return AB_BUFFER_OK;
}

/**
 * @brief 绑定两块存储并清空状态。
 *
 * Steps:
 *  1. 检查初始化参数。
 *  2. 初始化块描述和读写索引。
 *
 * @param[out] p_buffer   : 双缓冲对象。
 * @param[in] p_a         : A 块存储。
 * @param[in] p_b         : B 块存储。
 * @param[in] sz_capacity : 单块容量。
 *
 * @return ab_buffer_status_t : 初始化结果。
 */
ab_buffer_status_t ab_buffer_init(
    ab_buffer_t * const p_buffer,
    uint8_t * const     p_a,
    uint8_t * const     p_b,
    size_t              sz_capacity)
{
    //-----------------------------参数校验begin------------------------------//
    if ((NULL == p_buffer) || (NULL == p_a) || (NULL == p_b) ||
        (p_a == p_b) || (0U == sz_capacity))
    {
        return AB_BUFFER_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    p_buffer->ap_storage[0] = p_a;
    p_buffer->ap_storage[1] = p_b;
    p_buffer->sz_capacity = sz_capacity;
    p_buffer->asz_length[0] = 0U;
    p_buffer->asz_length[1] = 0U;
    p_buffer->ae_state[0] = AB_BUFFER_FREE;
    p_buffer->ae_state[1] = AB_BUFFER_FREE;
    p_buffer->sz_write_index = 0U;
    p_buffer->sz_read_index = 0U;
    return AB_BUFFER_OK;
}

/**
 * @brief 将空闲写块交给生产者，保留其他块的数据。
 *
 * Steps:
 *  1. 检查对象与输出参数。
 *  2. 仅在目标块空闲时移交写权限。
 *
 * @param[in,out] p_buffer : 双缓冲对象。
 * @param[out] pp_data     : 写地址。
 * @param[out] p_capacity  : 写容量。
 *
 * @return ab_buffer_status_t : 取得结果。
 */
ab_buffer_status_t ab_buffer_acquire_write(
    ab_buffer_t * const p_buffer,
    uint8_t ** const    pp_data,
    size_t * const      p_capacity)
{
    //-----------------------------参数校验begin------------------------------//
    if ((NULL == pp_data) || (NULL == p_capacity) ||
        (AB_BUFFER_OK != ab_buffer_validate(p_buffer)))
    {
        return AB_BUFFER_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    *pp_data = NULL;
    *p_capacity = 0U;
    if (AB_BUFFER_FREE != p_buffer->ae_state[p_buffer->sz_write_index])
    {
        return AB_BUFFER_BUSY;
    }
    p_buffer->ae_state[p_buffer->sz_write_index] = AB_BUFFER_WRITING;
    *pp_data = p_buffer->ap_storage[p_buffer->sz_write_index];
    *p_capacity = p_buffer->sz_capacity;
    return AB_BUFFER_OK;
}

/**
 * @brief 发布已完成的数据并切换写索引。
 *
 * Steps:
 *  1. 检查长度和写块所有权。
 *  2. 保存长度、标记就绪并切换到另一块。
 *
 * @param[in,out] p_buffer : 双缓冲对象。
 * @param[in] sz_length    : 有效字节数。
 *
 * @return ab_buffer_status_t : 提交结果。
 */
ab_buffer_status_t ab_buffer_commit_write(
    ab_buffer_t * const p_buffer,
    size_t              sz_length)
{
    //-----------------------------参数校验begin------------------------------//
    if ((AB_BUFFER_OK != ab_buffer_validate(p_buffer)) ||
        (0U == sz_length) || (sz_length > p_buffer->sz_capacity))
    {
        return AB_BUFFER_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    if (AB_BUFFER_WRITING !=
        p_buffer->ae_state[p_buffer->sz_write_index])
    {
        return AB_BUFFER_INVALID_STATE;
    }
    p_buffer->asz_length[p_buffer->sz_write_index] = sz_length;
    p_buffer->ae_state[p_buffer->sz_write_index] = AB_BUFFER_READY;
    p_buffer->sz_write_index = 1U - p_buffer->sz_write_index;
    return AB_BUFFER_OK;
}

/**
 * @brief 将最早提交且尚未读取的块交给消费者。
 *
 * Steps:
 *  1. 检查参数，区分读块占用和暂无数据。
 *  2. 标记读取中并输出数据地址及长度。
 *
 * @param[in,out] p_buffer : 双缓冲对象。
 * @param[out] pp_data     : 只读地址。
 * @param[out] p_length    : 有效字节数。
 *
 * @return ab_buffer_status_t : 取得结果。
 */
ab_buffer_status_t ab_buffer_acquire_read(
    ab_buffer_t * const   p_buffer,
    const uint8_t ** const pp_data,
    size_t * const        p_length)
{
    //-----------------------------参数校验begin------------------------------//
    if ((NULL == pp_data) || (NULL == p_length) ||
        (AB_BUFFER_OK != ab_buffer_validate(p_buffer)))
    {
        return AB_BUFFER_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    *pp_data = NULL;
    *p_length = 0U;
    if (AB_BUFFER_READING == p_buffer->ae_state[p_buffer->sz_read_index])
    {
        return AB_BUFFER_BUSY;
    }
    if (AB_BUFFER_READY != p_buffer->ae_state[p_buffer->sz_read_index])
    {
        return AB_BUFFER_EMPTY;
    }
    p_buffer->ae_state[p_buffer->sz_read_index] = AB_BUFFER_READING;
    *pp_data = p_buffer->ap_storage[p_buffer->sz_read_index];
    *p_length = p_buffer->asz_length[p_buffer->sz_read_index];
    return AB_BUFFER_OK;
}

/**
 * @brief 消费结束后归还读块。
 *
 * Steps:
 *  1. 检查对象及读块所有权。
 *  2. 清空长度、标记空闲并切换读索引。
 *
 * @param[in,out] p_buffer : 双缓冲对象。
 *
 * @return ab_buffer_status_t : 释放结果。
 */
ab_buffer_status_t ab_buffer_release_read(
    ab_buffer_t * const p_buffer)
{
    //-----------------------------参数校验begin------------------------------//
    if (AB_BUFFER_OK != ab_buffer_validate(p_buffer))
    {
        return AB_BUFFER_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    if (AB_BUFFER_READING != p_buffer->ae_state[p_buffer->sz_read_index])
    {
        return AB_BUFFER_INVALID_STATE;
    }
    p_buffer->asz_length[p_buffer->sz_read_index] = 0U;
    p_buffer->ae_state[p_buffer->sz_read_index] = AB_BUFFER_FREE;
    p_buffer->sz_read_index = 1U - p_buffer->sz_read_index;
    return AB_BUFFER_OK;
}
