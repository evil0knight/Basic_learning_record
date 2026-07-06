# typedef — 类型别名

[← C语言知识地图](./MOC.md)

---

`typedef` 的作用是给已有的类型起一个新名字。它不是创建新类型，只是增加一个别名。

## 1. 给基本类型起别名

```c
typedef unsigned int uint32_t;
```

从今往后，`uint32_t` 就等同于 `unsigned int`，可以直接用来声明变量：

```c
uint32_t a = 0;  // 等价于 unsigned int a = 0;
```

## 2. 给结构体起别名

不用 `typedef` 时，每次声明结构体变量都要带 `struct` 关键字：

```c
struct Animal {
    int age;
};
struct Animal dog;  // 必须写 struct
```

用 `typedef` 省掉 `struct`：

```c
typedef struct {
    int age;
} Animal_t;

Animal_t dog;  // 直接用，不用写 struct
```

也可以保留结构体标签，两种别名同时存在：

```c
typedef struct Animal {
    int age;
} Animal_t;

struct Animal cat;  // OK，用标签
Animal_t dog;       // OK，用别名
```

## 3. 给函数指针起别名

函数指针的原型写法非常冗长。假设有一个函数：

```c
led_status_t my_function(uint32_t a, uint32_t b, proportion_t c);
```

指向它的指针变量：

```c
led_status_t (*my_ptr)(uint32_t, uint32_t, proportion_t);
```

用 `typedef` 把这一整串提取成一个**类型名**：

```c
typedef led_status_t (*pf_led_control_t)(uint32_t, uint32_t, proportion_t);
```

**关键**：加上 `typedef` 后，`pf_led_control_t` 从变量名变成了类型名，新名字被夹在 `*` 和 `)` 之间。此后声明同类函数指针就和声明普通变量一样：

```c
pf_led_control_t controller_1;
pf_led_control_t controller_2;
```

它也可以作为结构体成员的类型：

```c
typedef struct {
    pf_led_control_t set_led;
    pf_led_control_t get_led;
} bsp_led_driver_t;
```

---

### 通用记忆法则

把 `typedef` 当成一次**普通变量声明**，然后在新名字前面加上 `typedef`——原本声明的那个名字就成了类型名：

| 普通声明 | 加上 typedef 后 |
|---|---|
| `int a;` | `typedef int a;` → `a` 变成 `int` 的别名 |
| `struct { int x; } s;` | `typedef struct { int x; } s;` → `s` 变成该结构体的别名 |
| `int (*fp)(char);` | `typedef int (*fp)(char);` → `fp` 变成该函数指针类型的别名 |
