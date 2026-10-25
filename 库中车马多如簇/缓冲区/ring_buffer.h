/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file ring_buffer.h
 *
 * @par dependencies
 * - stddef.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 独立字节环形缓冲区；存储空间由调用者提供。
 *
 * Processing flow:
 *
 * 初始化 -> 写入字节流 -> 读取字节流。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef RING_BUFFER_H  /* 防止头文件重复包含。 */
#define RING_BUFFER_H

//********************************Includes*********************************//

#include <stddef.h>
#include <stdint.h>

//********************************Includes*********************************//

typedef enum
{
    RING_BUFFER_OK = 0,
    RING_BUFFER_INVALID_PARAM,
    RING_BUFFER_FULL,
    RING_BUFFER_EMPTY
} ring_buffer_status_t;

/* 字段只由本模块修改；并发访问由调用者串行化，包括查询和复位。 */
typedef struct
{
    uint8_t *p_storage;
    size_t sz_capacity;
    size_t sz_head;
    size_t sz_tail;
    size_t sz_count;
} ring_buffer_t;

/**
 * @brief 绑定存储并清空状态，不申请内存。
 *
 * Steps:
 *  1. 检查对象、存储和容量。
 *  2. 绑定存储并清空游标及计数。
 *
 * @param[out] p_ring     : 缓冲区对象。
 * @param[in] p_storage   : 独占存储，生命周期覆盖对象使用期。
 * @param[in] sz_capacity : 容量，单位为字节，必须大于零。
 *
 * @return ring_buffer_status_t : 初始化结果。
 *
 */
ring_buffer_status_t ring_buffer_init(
    ring_buffer_t * const p_ring,
    uint8_t * const       p_storage,
    size_t                sz_capacity);

/**
 * @brief 整段写入；空间不足时不写入任何字节。
 *
 * Steps:
 *  1. 检查参数及剩余容量。
 *  2. 整段复制字节并更新写游标和计数。
 *
 * @param[in,out] p_ring : 已初始化对象。
 * @param[in] p_data     : 源数据，不得与环形存储重叠。
 * @param[in] sz_length  : 写入长度，必须大于零。
 *
 * @return ring_buffer_status_t : 成功、满或参数错误。
 *
 */
ring_buffer_status_t ring_buffer_write(
    ring_buffer_t * const p_ring,
    const uint8_t * const p_data,
    size_t                sz_length);

/**
 * @brief 读取至多 sz_capacity 字节，允许不足一整块。
 *
 * Steps:
 *  1. 检查参数并确认是否有数据。
 *  2. 取出可用字节并更新读游标和计数。
 *
 * @param[in,out] p_ring  : 已初始化对象。
 * @param[out] p_data     : 目标存储，不得与环形存储重叠。
 * @param[in] sz_capacity : 目标容量，必须大于零。
 * @param[out] p_length   : 实际读取长度；空时为零。
 *
 * @return ring_buffer_status_t : 成功、空或参数错误。
 *
 */
ring_buffer_status_t ring_buffer_read(
    ring_buffer_t * const p_ring,
    uint8_t * const       p_data,
    size_t                sz_capacity,
    size_t * const        p_length);

/**
 * @brief 丢弃全部未读数据。
 *
 * Steps:
 *  1. 检查缓冲对象。
 *  2. 清空读写游标和未读计数。
 *
 * @param[in,out] p_ring : 已初始化对象。
 *
 * @return ring_buffer_status_t : 复位结果。
 *
 */
ring_buffer_status_t ring_buffer_reset(
    ring_buffer_t * const p_ring);

#endif
