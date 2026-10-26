# 内存池

[← 返回缓冲区](../MOC.md) | [← 主页](../../../index.md)

---

调用者提供存储的定长块内存池，独立于 HAL 和 OS，不申请堆内存。

## 关键文件

| 文件 | 作用 |
| --- | --- |
| [memory_pool.h](./memory_pool.h) | 状态、对象及初始化、分配、释放、空闲块查询接口 |
| [memory_pool.c](./memory_pool.c) | 空闲块索引链表及块地址校验实现 |

## 使用约束

- 初始化传入独占存储、块大小和块数量；存储需至少容纳 `块大小 × 块数量` 字节，块大小至少为 `sizeof(size_t)`。对象和存储的生命周期必须覆盖整个使用期。
- 空闲块的前 `sizeof(size_t)` 字节保存链表索引；分配后整块空间均交给调用者。初始化会覆盖每个块的前缀。
- `memory_pool_free()` 仅接受本池返回且尚未释放的块首地址；其他地址、块内地址和重复释放返回 `MEMORY_POOL_INVALID_BLOCK`。
- `memory_pool_alloc()` 在池空时返回 `MEMORY_POOL_EMPTY` 并将输出指针置为 `NULL`。
- 模块不加锁；并发分配、释放和查询由调用者串行化。
- 对象、存储和输出参数之间不得重叠；对象字段只允许本模块修改。对象必须先成功初始化，不能把未初始化的自动变量直接传给其他接口。
- 模块通过 `memcpy()` 读写索引，对字节存储不额外要求对齐。若将块用于特定数据类型，调用者需保证每块的对齐和 C 对象类型访问规则；下例仅演示字节数据。
- 释放后不能再访问该块；模块不清零数据，也不能识别块重新分配后的旧指针。存在未归还块时不得重新初始化。

分配和空闲块查询为 O(1)，初始化和释放最坏为 O(N)，N 为块数量。释放通过遍历块首地址及空闲链表校验来源和重复释放，不需要额外的状态数组。参数检查不替代调用者提供真实存储容量的责任，也不保证检测所有越界写造成的破坏。

## 调用顺序

1. `memory_pool_init()` 建立空闲块链表。
2. `memory_pool_alloc()` 获取一个固定大小的块。
3. 使用完毕后用 `memory_pool_free()` 归还同一个块首地址。
4. 需要统计剩余块时调用 `memory_pool_get_free_count()`。

## 字节块示例

把 `.c` 加入工程，把 `.h` 所在目录加入头文件搜索路径。标准 C 库需提供 `memcpy()`。

```c
#include "memory_pool.h"

/**
 * @brief 演示单次初始化、分配、写入和归还。
 *
 * Steps:
 *  1. 使用栈上存储初始化四个定长块。
 *  2. 分配一个块、写入首字节并归还。
 *
 * @return memory_pool_status_t : 各步骤的状态。
 */
memory_pool_status_t memory_pool_example(void)
{
    memory_pool_t st_pool = {0};
    uint8_t au8_storage[4U * 32U];
    void *p_block = NULL;
    uint8_t *p_data;
    memory_pool_status_t e_status;

    e_status = memory_pool_init(&st_pool, au8_storage, 32U, 4U);
    if (MEMORY_POOL_OK != e_status)
    {
        return e_status;
    }
    e_status = memory_pool_alloc(&st_pool, &p_block);
    if (MEMORY_POOL_OK != e_status)
    {
        return e_status;
    }
    p_data = p_block;
    p_data[0] = 0x5AU;
    return memory_pool_free(&st_pool, p_block);
}
```
