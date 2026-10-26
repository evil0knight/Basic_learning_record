/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file memory_pool.h
 *
 * @par dependencies
 * - stddef.h
 * - stdint.h
 *
 * @author YA
 *
 * @brief 调用者提供存储的定长块内存池，不申请堆内存。
 *
 * Processing flow:
 *
 * 初始化 -> 分配定长块 -> 释放定长块。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

//********************************Includes*********************************//

#include <stddef.h>
#include <stdint.h>

//********************************Includes*********************************//

typedef enum
{
    MEMORY_POOL_OK = 0,
    MEMORY_POOL_INVALID_PARAM,
    MEMORY_POOL_EMPTY,
    MEMORY_POOL_INVALID_BLOCK
} memory_pool_status_t;

/* 字段仅由本模块修改；包括查询在内的所有访问由调用者串行化。 */
/* 块大小至少 sizeof(size_t)；空闲时借用块头保存下一个块的索引。 */
typedef struct
{
    uint8_t *p_storage;
    size_t sz_block_size;
    size_t sz_block_count;
    size_t sz_free_head;
    size_t sz_free_count;
} memory_pool_t;

/**
 * @brief 绑定存储并建立空闲块链表，不申请内存。
 *
 * Steps:
 *  1. 检查对象、存储、块尺寸和块数量。
 *  2. 在各块头部写入下一个空闲块索引。
 *
 * @param[out] p_pool         : 内存池对象。
 * @param[in] p_storage       : 独占存储，容量至少为块大小乘以块数。
 * @param[in] sz_block_size   : 块大小，至少 sizeof(size_t) 字节。
 * @param[in] sz_block_count  : 块数量，必须大于零。
 *
 * @return memory_pool_status_t : 初始化结果。
 *
 */
memory_pool_status_t memory_pool_init(
    memory_pool_t * const p_pool,
    uint8_t * const       p_storage,
    size_t                sz_block_size,
    size_t                sz_block_count);

/**
 * @brief 从空闲链表分配一个定长块。
 *
 * Steps:
 *  1. 检查内存池对象及其空闲链表状态。
 *  2. 弹出空闲块并返回块首地址。
 *
 * @param[in,out] p_pool  : 已初始化的内存池。
 * @param[out] pp_block   : 接收块首地址；有效参数下池空时写入 NULL。
 *
 * @return memory_pool_status_t : 成功、池空或参数错误。
 *
 */
memory_pool_status_t memory_pool_alloc(
    memory_pool_t * const p_pool,
    void ** const         pp_block);

/**
 * @brief 归还属于内存池的块首地址。
 *
 * Steps:
 *  1. 检查指针位于池内、按块对齐且尚未释放。
 *  2. 将块压入空闲链表。
 *
 * @param[in,out] p_pool  : 已初始化的内存池。
 * @param[in] p_block     : 本池分配且尚未释放的块首地址。
 *
 * @return memory_pool_status_t : 成功、无效块或参数错误。
 *
 */
memory_pool_status_t memory_pool_free(
    memory_pool_t * const p_pool,
    void * const          p_block);

/**
 * @brief 查询尚未分配的块数。
 *
 * Steps:
 *  1. 检查对象和输出指针。
 *  2. 返回当前空闲块数。
 *
 * @param[in] p_pool         : 已初始化的内存池。
 * @param[out] p_free_count  : 接收空闲块数。
 *
 * @return memory_pool_status_t : 查询结果。
 *
 */
memory_pool_status_t memory_pool_get_free_count(
    const memory_pool_t * const p_pool,
    size_t * const              p_free_count);

#endif
