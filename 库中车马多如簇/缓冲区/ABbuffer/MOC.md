# ABbuffer

[← MOC](../MOC.md) | [← 主页](../../../index.md)

---

独立的零拷贝 AB 双缓冲，放入工程 `06_middlewares`。调用者提供两块等容量存储，不申请堆内存，无 HAL / OS 依赖。

## 关键文件

| 文件 | 作用 |
| --- | --- |
| [ab_buffer.h](./ab_buffer.h) | 状态、对象和生产者 / 消费者接口 |
| [ab_buffer.c](./ab_buffer.c) | 块所有权及读写索引切换 |

## 交接流程

每块独立经历 `FREE → WRITING → READY → READING → FREE`。生产者和消费者各自按 A、B、A、B 的顺序推进，保持提交顺序。

| 接口 | 行为 |
| --- | --- |
| `ab_buffer_init()` | 绑定存储，两块均置为空闲，从 A 开始 |
| `ab_buffer_acquire_write()` | 取得写地址和整块容量，目标非空闲时返回 `AB_BUFFER_BUSY` |
| `ab_buffer_commit_write()` | 提交实际有效长度，发布可读状态，切换写索引 |
| `ab_buffer_acquire_read()` | 取得最早提交的只读地址及长度；无数据返回 `AB_BUFFER_EMPTY`，重复取得未释放读块返回 `AB_BUFFER_BUSY` |
| `ab_buffer_release_read()` | 处理完成后释放整块，切换读索引 |

A 正在读取时可以取得 B 写入。两块均占用时暂停生产，不覆盖未处理数据。取得读块后可分多次处理，完成后仅释放一次；模块不维护块内读取偏移。

## 使用约束

- 两块存储各至少包含指定容量，彼此以及与对象、接口输出参数均不重叠。初始化只能检查空指针、相同起始地址和零容量，实际大小及部分重叠由调用者保证。
- 对象须先成功初始化，使用期内保持对象和存储有效，字段只允许本模块修改。存在未完成的读写或 DMA 操作时不得重新初始化。
- 一次只允许持有一个写块和一个读块。提交前完成全部写入；提交后禁止继续访问原写地址。释放前完成全部读取；释放后禁止继续访问原读地址。
- 有效提交长度为 `1..sz_capacity`；非法长度不会交出写块，可修正后重新提交。此版本不提供取消写入或丢弃数据接口。
- 全部接口调用必须由上层串行化，包括中断与任务之间的调用；模块不使用锁、原子变量或内存屏障。取得所有权后的数据访问可在接口临界区之外进行，生产者与消费者只能访问各自持有的块。
- DMA 使用写地址前由上层配置并启动，完成后才能提交；DMA 消费读块时须等待传输完成后再释放。硬件对齐、缓存清理 / 失效、内存屏障及可访问区域由上层负责。模块不会自动切换硬件 DMA 地址，硬件不得绕过忙状态持续覆盖两块存储。
- 不清零存储，只有提交的有效长度可读。所有接口为 O(1)，不复制数据。

## 字节数据示例

将 `.c` 加入工程，将头文件目录加入搜索路径。示例完整展示一次交接；持续运行时生产者和消费者分别重复各自的取得及交还操作。

```c
#include "ab_buffer.h"

/**
 * @brief 演示单次 AB 双缓冲交接。
 *
 * Steps:
 *  1. 初始化并取得写块，写入一个字节后提交。
 *  2. 取得读块并验证数据，最后释放。
 *
 * @return ab_buffer_status_t : 操作结果。
 */
ab_buffer_status_t ab_buffer_example(void)
{
    ab_buffer_t st_buffer = {0};
    uint8_t au8_a[64];
    uint8_t au8_b[64];
    uint8_t *p_write = NULL;
    const uint8_t *p_read = NULL;
    size_t sz_capacity = 0U;
    size_t sz_length = 0U;
    ab_buffer_status_t e_status;

    e_status = ab_buffer_init(&st_buffer, au8_a, au8_b, sizeof(au8_a));
    if (AB_BUFFER_OK != e_status)
    {
        return e_status;
    }
    e_status = ab_buffer_acquire_write(&st_buffer, &p_write,
                                        &sz_capacity);
    if (AB_BUFFER_OK != e_status)
    {
        return e_status;
    }
    p_write[0] = 0x5AU;
    e_status = ab_buffer_commit_write(&st_buffer, 1U);
    if (AB_BUFFER_OK != e_status)
    {
        return e_status;
    }
    e_status = ab_buffer_acquire_read(&st_buffer, &p_read, &sz_length);
    if (AB_BUFFER_OK != e_status)
    {
        return e_status;
    }
    if ((1U != sz_length) || (0x5AU != p_read[0]))
    {
        (void)ab_buffer_release_read(&st_buffer);
        return AB_BUFFER_INVALID_STATE;
    }
    return ab_buffer_release_read(&st_buffer);
}
```
