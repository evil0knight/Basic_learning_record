/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file ab_buffer.h
 *
 * @par dependencies
 * - stddef.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 调用者提供两块存储的零拷贝 AB 双缓冲。
 *
 * Processing flow:
 *
 * 初始化 -> 取得写块 -> 提交 -> 取得读块 -> 释放。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef AB_BUFFER_H
#define AB_BUFFER_H

//********************************Includes*********************************//

#include <stddef.h>
#include <stdint.h>

//********************************Includes*********************************//

typedef enum
{
    AB_BUFFER_OK = 0,
    AB_BUFFER_INVALID_PARAM,
    AB_BUFFER_BUSY,
    AB_BUFFER_EMPTY,
    AB_BUFFER_INVALID_STATE
} ab_buffer_status_t;

typedef enum
{
    AB_BUFFER_FREE = 0,
    AB_BUFFER_WRITING,
    AB_BUFFER_READY,
    AB_BUFFER_READING
} ab_buffer_block_state_t;

/* 字段仅由模块修改；所有接口调用必须由上层串行化。 */
typedef struct
{
    uint8_t *ap_storage[2];
    size_t sz_capacity;
    size_t asz_length[2];
    ab_buffer_block_state_t ae_state[2];
    size_t sz_write_index;
    size_t sz_read_index;
} ab_buffer_t;

/**
 * @brief 绑定两块等容量存储，初始化为全部空闲。
 *
 * Steps:
 *  1. 检查对象、两块存储和容量。
 *  2. 绑定存储，清空长度、状态及读写索引。
 *
 * @param[out] p_buffer  : 双缓冲对象。
 * @param[in] p_a        : A 块独占存储。
 * @param[in] p_b        : B 块独占存储，不得与 A 块重叠。
 * @param[in] sz_capacity : 每块字节容量，必须大于零。
 *
 * @return ab_buffer_status_t : 初始化结果。
 */
ab_buffer_status_t ab_buffer_init(
    ab_buffer_t * const p_buffer,
    uint8_t * const     p_a,
    uint8_t * const     p_b,
    size_t              sz_capacity);

/**
 * @brief 取得当前空闲写块，仅允许一个未提交的写块。
 *
 * Steps:
 *  1. 检查对象、输出参数和目标块状态。
 *  2. 将目标块标记为写入中并输出地址及容量。
 *
 * @param[in,out] p_buffer : 已初始化对象。
 * @param[out] pp_data     : 写块地址，忙时为 NULL。
 * @param[out] p_capacity  : 写块容量，忙时为零。
 *
 * @return ab_buffer_status_t : 成功、忙或参数错误。
 */
ab_buffer_status_t ab_buffer_acquire_write(
    ab_buffer_t * const p_buffer,
    uint8_t ** const    pp_data,
    size_t * const      p_capacity);

/**
 * @brief 提交写块，使其可读，并切换下一个写索引。
 *
 * Steps:
 *  1. 检查长度和当前块是否处于写入中。
 *  2. 保存有效长度，发布就绪状态并切换写索引。
 *
 * @param[in,out] p_buffer : 已初始化对象。
 * @param[in] sz_length    : 实际有效字节数，大于零且不超过容量。
 *
 * @return ab_buffer_status_t : 提交结果；失败时仍持有写块。
 */
ab_buffer_status_t ab_buffer_commit_write(
    ab_buffer_t * const p_buffer,
    size_t              sz_length);

/**
 * @brief 按提交顺序取得读块，不复制数据。
 *
 * Steps:
 *  1. 检查输出参数及目标块状态。
 *  2. 标记为读取中并输出只读地址和有效长度。
 *
 * @param[in,out] p_buffer : 已初始化对象。
 * @param[out] pp_data     : 只读块地址，无数据或忙时为 NULL。
 * @param[out] p_length    : 有效长度，无数据或忙时为零。
 *
 * @return ab_buffer_status_t : 成功、空、忙或参数错误。
 */
ab_buffer_status_t ab_buffer_acquire_read(
    ab_buffer_t * const   p_buffer,
    const uint8_t ** const pp_data,
    size_t * const        p_length);

/**
 * @brief 释放正在读取的块，允许生产者再次使用。
 *
 * Steps:
 *  1. 检查当前块是否处于读取中。
 *  2. 清空长度、标记空闲并切换读索引。
 *
 * @param[in,out] p_buffer : 已初始化对象。
 *
 * @return ab_buffer_status_t : 释放结果。
 */
ab_buffer_status_t ab_buffer_release_read(
    ab_buffer_t * const p_buffer);

#endif
