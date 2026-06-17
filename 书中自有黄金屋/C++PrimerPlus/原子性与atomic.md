# 原子性与 std::atomic

[← 并发编程](./并发编程.md) | [← C++ 知识地图](./MOC.md) | [← volatile](./volatile.md)

---

## 什么是原子性

**原子操作**：不可被中断的操作，要么完全执行，要么完全不执行，中间不会被其他线程/中断打断。

```c
// 非原子：i++ 实际是三步（读 → 加 → 写），多线程下会出错
int i = 0;
i++;  // 两个线程各执行一次 i++，期望结果是 2，但因为竞态可能只得到 1

// 原子：硬件保证整个操作不可分割
std::atomic<int> i = 0;
i++;  // 安全
```

---

## 为什么需要原子性

| 问题                      | 原因                               |
| ------------------------- | ---------------------------------- |
| 竞态条件 (Race Condition) | 多线程同时读写同一变量，结果不确定 |
| 指令重排                  | 编译器/CPU 可能打乱指令顺序        |
| 缓存不一致                | 多核 CPU 各自缓存变量，写入不可见  |

---

## std::atomic 基本用法

```cpp
#include <atomic>

std::atomic<int>  counter{0};
std::atomic<bool> flag{false};

// 常用操作
counter++;              // 原子自增
counter.fetch_add(5);   // 原子加，返回旧值
int val = counter.load();       // 原子读
counter.store(10);              // 原子写
counter.exchange(20);           // 原子交换，返回旧值

// CAS：Compare And Swap（无锁编程核心）
int expected = 5;
bool ok = counter.compare_exchange_strong(expected, 10);
// 若 counter == expected，则写入10，返回true
// 否则将 expected 更新为当前值，返回false
```

---

## 内存顺序 (Memory Order)

控制原子操作的可见性和重排限制，性能从低到高：

| 参数                     | 含义                     | 场景                  |
| ------------------------ | ------------------------ | --------------------- |
| `memory_order_seq_cst` | 全局顺序一致（默认）     | 最安全，性能最低      |
| `memory_order_acquire` | 读屏障，之后的读写不上移 | 配合 release 用于同步 |
| `memory_order_release` | 写屏障，之前的读写不下移 | 配合 acquire 用于同步 |
| `memory_order_relaxed` | 无顺序保证，只保证原子性 | 计数器等无依赖场景    |

```cpp
// 典型 acquire-release 同步模式
std::atomic<bool> ready{false};
int data = 0;

// 生产者线程
data = 42;
ready.store(true, std::memory_order_release);  // 保证 data=42 在前

// 消费者线程
while (!ready.load(std::memory_order_acquire));  // 保证读到 data=42
assert(data == 42);  // 安全
```

---

## volatile vs atomic

|                | `volatile`    | `std::atomic` |
| -------------- | --------------- | --------------- |
| 禁止编译器缓存 | ✅              | ✅              |
| 保证原子性     | ❌              | ✅              |
| 保证内存顺序   | ❌              | ✅              |
| 适用场景       | 硬件寄存器、ISR | 多线程共享变量  |

> 嵌入式裸机：寄存器用 `volatile`；有 RTOS 多任务：共享变量用 `atomic` 或互斥锁。
