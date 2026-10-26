/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file memory_pool.c
 *
 * @par dependencies
 * - memory_pool.h
 * - string.h
 *
 * @author YA
 *
 * @brief 定长块内存池实现，无 HAL、OS 或堆分配依赖。
 *
 * Processing flow:
 *
 * 建立空闲链表 -> 分配块 -> 校验并归还块。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "memory_pool.h"

#include <string.h>

/**
 * @brief 检查存储参数及空闲链表头和计数是否有效。
 *
 * Steps:
 *  1. 检查对象、存储及容量乘法是否溢出。
 *  2. 检查空闲块数和链表头的一致性。
 *
 * @param[in] p_pool : 内存池对象。
 *
 * @return memory_pool_status_t : 检查结果。
 *
 */
static memory_pool_status_t memory_pool_validate(
    const memory_pool_t * const p_pool)
{
    //-----------------------------参数校验begin------------------------------//
    if ((NULL == p_pool) || (NULL == p_pool->p_storage) ||
        (p_pool->sz_block_size < sizeof(size_t)) ||
        (0U == p_pool->sz_block_count))
    {
        return MEMORY_POOL_INVALID_PARAM;
    }
    if ((p_pool->sz_block_count >
         (SIZE_MAX / p_pool->sz_block_size)) ||
        (p_pool->sz_free_count > p_pool->sz_block_count) ||
        (p_pool->sz_free_head > p_pool->sz_block_count))
    {
        return MEMORY_POOL_INVALID_PARAM;
    }
    if (((0U == p_pool->sz_free_count) &&
         (p_pool->sz_free_head != p_pool->sz_block_count)) ||
        ((0U != p_pool->sz_free_count) &&
         (p_pool->sz_free_head == p_pool->sz_block_count)))
    {
        return MEMORY_POOL_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    return MEMORY_POOL_OK;
}

/**
 * @brief 将外部存储分为等大的块并串联全部空闲块。
 *
 * Steps:
 *  1. 检查参数及总容量乘法。
 *  2. 写入链表索引并初始化对象。
 *
 * @param[out] p_pool        : 内存池对象。
 * @param[in] p_storage      : 生命周期覆盖内存池使用期的存储。
 * @param[in] sz_block_size  : 每块字节数，至少 sizeof(size_t)。
 * @param[in] sz_block_count : 块数，必须大于零。
 *
 * @return memory_pool_status_t : 初始化结果。
 *
 */
memory_pool_status_t memory_pool_init(
    memory_pool_t * const p_pool,
    uint8_t * const       p_storage,
    size_t                sz_block_size,
    size_t                sz_block_count)
{
    size_t sz_index;
    size_t sz_next;

    //-----------------------------参数校验begin------------------------------//
    if ((NULL == p_pool) || (NULL == p_storage) ||
        (sz_block_size < sizeof(size_t)) || (0U == sz_block_count))
    {
        return MEMORY_POOL_INVALID_PARAM;
    }
    if (sz_block_count > (SIZE_MAX / sz_block_size))
    {
        return MEMORY_POOL_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    /* 用 memcpy 保存索引，避免把字节存储强转为 size_t 指针。 */
    for (sz_index = 0U; sz_index < sz_block_count; ++sz_index)
    {
        sz_next = sz_index + 1U;
        (void)memcpy(&p_storage[sz_index * sz_block_size],
                     &sz_next, sizeof(sz_next));
    }
    p_pool->p_storage = p_storage;
    p_pool->sz_block_size = sz_block_size;
    p_pool->sz_block_count = sz_block_count;
    p_pool->sz_free_head = 0U;
    p_pool->sz_free_count = sz_block_count;
    return MEMORY_POOL_OK;
}

/**
 * @brief 弹出空闲链表头，允许使用分配块的全部字节。
 *
 * Steps:
 *  1. 检查输出指针和内存池状态。
 *  2. 读取下一个索引并交出当前块。
 *
 * @param[in,out] p_pool : 已初始化的内存池。
 * @param[out] pp_block  : 块首地址，池空时为 NULL。
 *
 * @return memory_pool_status_t : 分配结果。
 *
 */
memory_pool_status_t memory_pool_alloc(memory_pool_t * const p_pool,
                                       void ** const         pp_block)
{
    size_t sz_next;
    uint8_t *p_block;

    //-----------------------------参数校验begin------------------------------//
    if ((NULL == pp_block) ||
        (MEMORY_POOL_OK != memory_pool_validate(p_pool)))
    {
        return MEMORY_POOL_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    *pp_block = NULL;
    if (0U == p_pool->sz_free_count)
    {
        return MEMORY_POOL_EMPTY;
    }
    /* 块索引乘以块大小得到字节偏移，定位当前空闲链表头块。 */
    p_block = &p_pool->p_storage[p_pool->sz_free_head * p_pool->sz_block_size];
    
    /* 空闲块头部存有下一块索引；分配前先取出，避免被用户数据覆盖。 */
    /* memcpy 不要求块地址按 size_t 对齐；(void) 表示忽略其返回值。 */
    (void)memcpy(&sz_next, p_block, sizeof(sz_next));
    /* 先检查链表索引，失败时保留原状态。块数也是链尾标记。 */
    if (((1U == p_pool->sz_free_count) &&
         (sz_next != p_pool->sz_block_count)) ||
        ((1U < p_pool->sz_free_count) &&
         ((sz_next >= p_pool->sz_block_count) ||
          (sz_next == p_pool->sz_free_head))))
    {
        return MEMORY_POOL_INVALID_PARAM;
    }
    /* 链表头移向下一空闲块，当前块从空闲链表中摘除。 */
    p_pool->sz_free_head = sz_next;
    /* 已分配一块，剩余空闲块数减一。 */
    --p_pool->sz_free_count;
    /* 通过二级指针返回当前块首地址，调用者可使用整块空间。 */
    *pp_block = p_block;
    return MEMORY_POOL_OK;
}

/**
 * @brief 校验精确块首地址并防止重复释放，再插入空闲链表。
 *
 * Steps:
 *  1. 遍历块首地址，拒绝外来指针和块内偏移地址。
 *  2. 遍历空闲链表，拒绝重复释放及非法链表索引。
 *  3. 写入空闲链表头并增加空闲计数。
 *
 * @param[in,out] p_pool : 已初始化的内存池。
 * @param[in] p_block    : 待释放的块首地址。
 *
 * @return memory_pool_status_t : 释放结果。
 *
 */
memory_pool_status_t memory_pool_free(memory_pool_t * const p_pool,
                                      void * const          p_block)
{
    size_t sz_index;
    size_t sz_target;
    size_t sz_current;
    size_t sz_step;

    //-----------------------------参数校验begin------------------------------//
    if ((NULL == p_block) ||
        (MEMORY_POOL_OK != memory_pool_validate(p_pool)))
    {
        return MEMORY_POOL_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    sz_target = p_pool->sz_block_count;
    /* 仅比较地址相等，不对可能来自其他对象的指针作减法或排序。 */
    for (sz_index = 0U; sz_index < p_pool->sz_block_count; ++sz_index)
    {
        if (p_block == (void *)&p_pool->p_storage[
                sz_index * p_pool->sz_block_size])
        {
            sz_target = sz_index;
            break;
        }
    }
    if (sz_target == p_pool->sz_block_count)
    {
        return MEMORY_POOL_INVALID_BLOCK;
    }
    sz_current = p_pool->sz_free_head;
    for (sz_step = 0U; sz_step < p_pool->sz_free_count; ++sz_step)
    {
        if (sz_current >= p_pool->sz_block_count)
        {
            return MEMORY_POOL_INVALID_PARAM;
        }
        if (sz_current == sz_target)
        {
            return MEMORY_POOL_INVALID_BLOCK;
        }
        (void)memcpy(&sz_current, &p_pool->p_storage[
            sz_current * p_pool->sz_block_size], sizeof(sz_current));
    }
    if ((sz_current != p_pool->sz_block_count) ||
        (p_pool->sz_free_count == p_pool->sz_block_count))
    {
        return MEMORY_POOL_INVALID_PARAM;
    }
    (void)memcpy(p_block, &p_pool->sz_free_head,
                 sizeof(p_pool->sz_free_head));
    p_pool->sz_free_head = sz_target;
    ++p_pool->sz_free_count;
    return MEMORY_POOL_OK;
}

/**
 * @brief 返回当前尚未分配的块数。
 *
 * Steps:
 *  1. 检查对象和输出参数。
 *  2. 输出空闲计数。
 *
 * @param[in] p_pool        : 已初始化的内存池。
 * @param[out] p_free_count : 接收空闲块数。
 *
 * @return memory_pool_status_t : 查询结果。
 *
 */
memory_pool_status_t memory_pool_get_free_count(
    const memory_pool_t * const p_pool,
    size_t * const              p_free_count)
{
    //-----------------------------参数校验begin------------------------------//
    if ((NULL == p_free_count) ||
        (MEMORY_POOL_OK != memory_pool_validate(p_pool)))
    {
        return MEMORY_POOL_INVALID_PARAM;
    }
    //------------------------------参数校验end-------------------------------//
    *p_free_count = p_pool->sz_free_count;
    return MEMORY_POOL_OK;
}
