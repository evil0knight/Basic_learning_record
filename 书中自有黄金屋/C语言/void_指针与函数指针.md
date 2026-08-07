# void* 与 void(*)(void)

[← C语言知识地图](./MOC.md)

---

## void* — 通用指针

**核心：可以指向任何类型，C 中任意指针自动转为 `void*`，反过来须显式强转（C++ 两边都须强转）。**

`void*` 是一种"无类型"指针，仅保存地址，不携带指向类型的信息。

### 隐式转换（C only）

```c
int   a = 42;
void *p = &a;                    // ✅ 任意类型 → void*，无需强转
int  *pi = (int *)p;             // ⚠️ void* → 具体类型，必须强转
```

> C++ 两个方向都须显式转换，C 设计更宽松。

### 不可解引用、不可算术运算

`void` 是不完整类型，编译器不知道 sizeof，因此：

```c
void *p = &a;
*p;          // ❌ 编译错误：不能解引用 void*
p++;         // ❌ 编译错误：不知道步长（GCC 扩展允许，等价 +1，不推荐）
```

### 典型场景

| 场景                        | 示例                                         |
| --------------------------- | -------------------------------------------- |
| 动态内存分配                | `malloc` 返回 `void*`                        |
| 泛型回调                    | `qsort` / `bsearch` 的比较函数参数            |
| 线程/任务参数               | `pthread_create` 的 `void *arg`              |
| 消除类型依赖                | 头文件只暴露 `void*` 句柄，隐藏实现           |

```c
// malloc：不知道你要存什么，返回 void*
int *arr = (int *)malloc(100 * sizeof(int));

// qsort：通用排序，比较函数通过 void* 拿到实际数据
int cmp_int(const void *a, const void *b) {
    return *(int *)a - *(int *)b;
}
qsort(arr, n, sizeof(int), cmp_int);
```

---

## void(*)(void) — 函数指针（无参无返回值）

**核心：这是一个函数指针类型，指向"没有参数、没有返回值"的函数。**

回想函数指针声明规则（见 [函数与指针](函数与指针.md)）：

```c
void (*fp)(void);  // fp 是指针 → 指向 void(void) 类型的函数
```

### 为什么分两处写 void

```c
void  (*fp)  (void);
 ↑      ↑       ↑
 |      |       参数列表：void = 没有参数（C 中可省略，但不推荐）
 |      fp 是函数指针
 返回值：void = 没有返回值
```

### 常见用途

```c
// 1. 任务/回调注册（RTOS 任务入口、中断回调）
typedef void (*task_func_t)(void);

void led_blink(void) {
    // toggle LED
}

task_func_t task = led_blink;
task();  // 执行回调

// 2. 函数指针数组 — 状态机动作表
void state_idle(void)    { /* ... */ }
void state_active(void)  { /* ... */ }
void state_error(void)   { /* ... */ }

void (*state_table[])(void) = {state_idle, state_active, state_error};
state_table[current_state]();  // 执行当前状态动作
```

### typedef 化简

裸写 `void (*)(void)` 作为参数/返回类型时很绕，一律 `typedef`：

```c
typedef void (*callback_t)(void);  // callback_t 是类型名

callback_t fp = led_blink;         // 声明指针
void register(callback_t cb);      // 作参数
callback_t get_handler(void);      // 作返回值
```
