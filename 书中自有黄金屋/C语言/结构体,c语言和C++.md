# 结构体, C语言和C++

[← C语言知识地图](./MOC.md)

---

## 索引

1. [C语言里怎么创建结构体](#1-c语言里怎么创建结构体)
2. [typedef 到底解决了什么](#2-typedef-到底解决了什么)
3. [结构体数组、指针、嵌套](#3-结构体数组指针嵌套)
4. [C++ 里有哪些变化](#4-c-里有哪些变化)
5. [对齐、填充与 packed](#5-对齐填充与-packed)
6. [什么时候该用 struct](#6-什么时候该用-struct)

---

## 1. C语言里怎么创建结构体

结构体就是把多个有关联的数据打包成一个整体。

```c
struct Student {
    char name[20];
    int  age;
    float score;
};
```

这段代码只是**定义了一种结构体类型**，还没有创建变量。

### 1.1 创建结构体变量

```c
struct Student {
    char name[20];
    int  age;
    float score;
};

struct Student s1;
struct Student s2 = {"Tom", 18, 95.5f};
```

在 C 语言里，类型名是 `struct Student`，不是单独的 `Student`。

也就是说：

```c
Student s3;          // ❌ C语言里这样写不行
struct Student s3;   // ✅ 这样才对
```

### 1.3 结构体指针访问

```c
#include <stdio.h>

struct Point {
    int x;
    int y;
};

int main(void) {
    struct Point p = {10, 20};
    struct Point *pp = &p;

    printf("x = %d\n", pp->x);
    printf("y = %d\n", pp->y);

    return 0;
}
```

指针用 `->`，它等价于：

```c
(*pp).x
(*pp).y
```

---

## 2. typedef 到底解决了什么

C语言原生写法里，每次都要写 `struct 名字`，比较啰嗦，所以经常配合 `typedef` 使用。

### 2.1 常见写法

```c
typedef struct {
    char name[20];
    int  age;
    float score;
} Student;

int main(void) {
    Student s1 = {"Bob", 19, 91.0f};
    return 0;
}
```

这样以后就可以直接写 `Student s1;`，不用再写 `struct Student s1;`。

### 2.2 带标签的 typedef

```c
typedef struct Student {
    char name[20];
    int  age;
} Student;
```

这时有两种名字：

- `struct Student`：结构体标签名
- `Student`：typedef 起的别名

所以这两种都能用：

```c
struct Student a;
Student b;
```

---

## 4. C++ 里有哪些变化

C++ 仍然保留了 `struct`，但比 C 语言更强。

### 4.1 最直接的变化：可以省掉 `struct`

C++：

```cpp
struct Student {
    int age;
};

Student s1;
```

也就是说，C++ 会自动把结构体名当成类型名。

### 4.2 struct 里可以放函数

```cpp
#include <iostream>
using namespace std;

struct Student {
    const char *name;
    int age;

    void print() const {
        cout << name << ", " << age << endl;
    }
};
```

这在 C 语言里不行，C 的结构体只能放数据成员。

### 4.3 struct 可以有构造函数

```cpp
struct Point {
    int x;
    int y;

    Point(int a, int b) : x(a), y(b) {}
};
```

C语言没有构造函数，通常只能靠初始化列表、赋值、或者专门的初始化函数。

### 4.4 struct 和 class 的主要区别

在 C++ 里，`struct` 和 `class` 几乎一样，核心区别只有默认权限：

- `struct` 默认是 `public`
- `class` 默认是 `private`

例如：

```cpp
struct A {
    int x;   // 默认 public
};

class B {
    int x;   // 默认 private
};
```

所以在 C++ 里，`struct` 通常用来表示“偏数据”的对象，`class` 更常用于封装行为。

---

## 5. 对齐、填充与 packed

这是结构体在嵌入式、通信协议、文件格式里非常重要的一块。

### 5.1 为什么结构体大小会变大

```c
struct Example {
    char a;
    int  b;
    char c;
};
```

你以为大小是 `1 + 4 + 1 = 6`，但很多编译器下实际不是 6。

原因是：**编译器会为了访问效率做对齐（alignment）**，在成员之间插入填充字节（padding）。

一种常见布局可能是：

```text
a _ _ _ b b b b c _ _ _
```

于是 `sizeof(struct Example)` 可能是 12。

### 5.2 packed 是干什么的

在 GCC / Clang 里，经常会看到：

```c
struct __attribute__((packed)) Packet {
    unsigned char head;
    unsigned short len;
    unsigned char cmd;
};
```

或者：

```c
typedef struct __attribute__((packed)) {
    unsigned char head;
    unsigned short len;
    unsigned char cmd;
} Packet;
```

`packed` 的作用是：**尽量取消编译器自动插入的填充字节**，让结构体布局更贴近字节流本身。

这在下面场景很常见：

- 通信协议
- 寄存器映射
- 文件格式解析
- 网络报文

### 5.3 但 packed 不是 C++ 独有

`__attribute__((packed))` 不是标准 C，也不是标准 C++ 语法。
它通常是 **GCC / Clang 编译器扩展**，所以 C 和 C++ 都可能支持。

也就是说：

- 它**不是 C++ 新增关键字**
- 它是**编译器提供的扩展能力**
- 不同编译器写法可能不同

比如 MSVC(Microsoft Visual C++,微软家的编译器) 更常见的是：

```c
#pragma pack(push, 1)
struct Packet {
    unsigned char head;
    unsigned short len;
    unsigned char cmd;
};
#pragma pack(pop)
```

### 5.4 packed 使用注意

`packed` 虽然能让结构体更紧凑，但不要滥用。

原因：

1. 某些 CPU 访问未对齐数据会变慢
2. 某些平台甚至会直接异常
3. 不同编译器、不同平台的行为可能不同

所以更稳妥的原则是：

- **协议/寄存器映射**：按需使用 packed
- **普通业务结构体**：不要为了“省几个字节”强行 packed

---

## 小结

- C语言里定义结构体后，创建变量时通常要写 `struct 名字`
- `typedef` 的主要作用是给类型起别名，减少 `struct` 反复书写
- C++ 里结构体更强，可以直接当类型名，还能带函数、构造函数
- `__attribute__((packed))` 常见于协议和寄存器映射，但它是编译器扩展，不是 C++ 专属语法
- 结构体一旦涉及协议解析，就要特别注意**对齐、填充、跨平台差异**
