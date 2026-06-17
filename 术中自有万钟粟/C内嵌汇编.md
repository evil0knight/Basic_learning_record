# C 内嵌汇编

[← 返回 MOC](MOC.md)

> 在 C 代码中直接嵌入汇编指令，用于极致性能优化、访问特殊寄存器或执行 C 无法表达的硬件操作。

---

## 基本语法（GCC 内联汇编）

GCC 使用 `asm` 或 `__asm__` 关键字，完整格式：

```c
asm volatile (
    "汇编指令\n\t"
    : 输出操作数          // Output operands
    : 输入操作数          // Input operands
    : 破坏列表            // Clobber list
);
```

- `volatile`：禁止编译器对该段汇编做优化/重排
- `\n\t`：换行+缩进，保证生成的汇编文本格式正确
- 操作数约束见下表

---

## 操作数约束

| 约束符 | 含义                        |
| ------ | --------------------------- |
| `"r"`  | 任意通用寄存器              |
| `"m"`  | 内存操作数                  |
| `"i"`  | 立即数（编译期常量）        |
| `"=r"` | 输出到寄存器（只写）        |
| `"+r"` | 读写同一寄存器              |
| `"0"`  | 与第 0 个操作数使用同一位置 |

占位符：`%0` 第一个操作数，`%1` 第二个，依此类推。

---

## 读写普通变量

```c
#include <stdio.h>

int main(void) {
    int a = 5, b = 3, result;

    asm volatile (
        "add %1, %2, %0\n\t"   // result = a + b  (ARM 语法示例)
        : "=r"(result)          // 输出：result → %0
        : "r"(a), "r"(b)        // 输入：a → %1, b → %2
    );

    printf("result = %d\n", result);  // 8
    return 0;
}
```

> x86 示例（AT&T 语法，src→dst 顺序）：
> ```c
> asm volatile ("addl %1, %0" : "+r"(result) : "r"(b));
> ```

---

## 读写特殊寄存器（ARM Cortex-M 示例）

```c
// 读 CONTROL 寄存器
static inline uint32_t get_control(void) {
    uint32_t val;
    asm volatile ("MRS %0, CONTROL" : "=r"(val));
    return val;
}

// 写 CONTROL 寄存器
static inline void set_control(uint32_t val) {
    asm volatile ("MSR CONTROL, %0" :: "r"(val));
    asm volatile ("ISB");   // 指令同步屏障，确保立即生效
}
```

---

## 内存屏障

```c
// 编译器屏障：阻止编译器跨越此点重排内存访问
#define COMPILER_BARRIER() asm volatile ("" ::: "memory")

// ARM 数据内存屏障（DMB）
#define DMB() asm volatile ("DMB SY" ::: "memory")

// ARM 数据同步屏障（DSB）
#define DSB() asm volatile ("DSB SY" ::: "memory")
```

破坏列表中写 `"memory"` 告诉编译器：此处可能读写任意内存，不要缓存寄存器中的内存值。

---

## 开关全局中断（ARM Cortex-M）

```c
static inline void disable_irq(void) {
    asm volatile ("CPSID I" ::: "memory");
}

static inline void enable_irq(void) {
    asm volatile ("CPSIE I" ::: "memory");
}
```

---

## 无操作延时（NOP）

```c
static inline void nop(void) {
    asm volatile ("NOP");
}

// 循环延时（不精确，仅用于极短延时）
static inline void delay_nop(uint32_t n) {
    while (n--) {
        asm volatile ("NOP");
    }
}
```

---

## KEIL / ARMCC 语法差异

KEIL 的 ARMCC/AC6 编译器使用 `__asm` 关键字，语法略有不同：

```c
// ARMCC 内联汇编（不支持 GCC 约束语法）
__asm void disable_irq_keil(void) {
    CPSID I
    BX LR
}
```

> 现代 KEIL AC6 已支持 GCC 内联汇编语法（`__asm volatile(...)`），推荐统一用 GCC 风格。

---

## 常见坑

| 问题 | 原因 | 解决 |
| ---- | ---- | ---- |
| 编译器优化掉了汇编 | 没加 `volatile` | 加 `asm volatile` |
| 寄存器值被覆盖 | 没在破坏列表声明 | 在 clobber 里加寄存器名，如 `"r0"` |
| 内存操作不一致 | 编译器缓存了变量 | clobber 加 `"memory"` |
| ARM/x86 语法混用 | AT&T 与 Intel 语法不同 | 明确目标平台，x86 默认 AT&T |
