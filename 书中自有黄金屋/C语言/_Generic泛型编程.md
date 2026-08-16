# _Generic 泛型编程（C11）

[← C语言知识地图](./MOC.md)

---

C 没有函数重载，`_Generic` 是 C11 引入的**编译期类型分发**，用同一宏名按实参类型选不同实现，做出"类型泛型"效果（`<tgmath.h>` 就是靠它实现的）。

## 语法

```c
_Generic(控制表达式, 类型1: 结果表达式1, 类型2: 结果表达式2, ..., default: 默认结果)
```

## 工作方式

1. **只看类型**：取控制表达式的类型（**不求值**，类似 `sizeof`）
2. **匹配分发**：用该类型去关联列表找匹配项，返回对应的结果表达式
3. **无匹配**：没有 `default` 且类型对不上 → 编译错误

## 类型判定规则

| 规则                       | 说明                                                                                      |
| -------------------------- | ----------------------------------------------------------------------------------------- |
| 控制表达式**不求值** | 副作用不生效：`_Generic(x++, int:1, default:0)` 里 `x` 不会自增                       |
| 去除**顶层限定符**   | `const int` / `volatile int` 都按 `int` 匹配，不能同时列 `int:` 和 `const int:` |
| 数组→指针退化             | `int arr[10]` 的类型视为 `int*`                                                       |
| 关联类型不能兼容           | 两个关联写兼容类型（如 `int` 与 `signed`）→ 编译错误                                 |
| 未选中分支可"不存在"       | 只要求选中的结果表达式合法，未选中的不被求值/检查                                         |

## 典型用法

### ① 按类型选格式串

```c
#define MY_PRINT(x) _Generic((x), \
    int:    printf("%d\n", (x)), \
    double: printf("%f\n", (x)), \
    char*:  printf("%s\n", (x)), \
    default: printf("unknown type\n"))

MY_PRINT(42);      // 42
MY_PRINT(3.14);    // 3.140000
MY_PRINT("hello"); // hello
```

### ② 按类型选函数名（仿 `<tgmath.h>`）

```c
#define cbrt(X) _Generic((X), \
    long double: cbrtl, \
    default:     cbrt,  \
    float:       cbrtf)(X)
```

`_Generic` 的结果是个**函数名**，后面再 `(X)` 调用，实现"同名多态"。

### ③ 配合 typeof 做泛型运算

```c
#define MAX(a, b) _Generic((a), \
    long long: llmax, \
    double:    fmax,   \
    default:   imax)(a, b)
```
