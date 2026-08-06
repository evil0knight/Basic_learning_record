# typedef

[← C语言知识地图](./MOC.md)|[←C陷阱](..\C陷阱与缺陷\MOC.md)

---

`typedef` 是编译器处理的**类型别名**，不是创建新类型。三个主要作用：

- 给基本类型起别名
- 给结构体起别名
- 给函数指针起别名

```c
typedef unsigned int uint32_t;           // 基本类型别名

typedef struct { int x; int y; } Point;  // 结构体别名，之后不用写 struct
Point p1;                                // 直接用

typedef int (*func_ptr_t)(char);         // 函数指针别名
func_ptr_t fp;                           // 声明函数指针就像普通变量
```

**写出一个普通的变量声明，然后在最前面加上 `typedef`，原本的“变量名”就变成为了“新的类型别名”**

---

## 与 `#define`做对比

`typedef`是编译器处理的**类型别名**,而 `define`是预处理文本

```c
#define pInt int*
pInt a, b;      // → int *a, b;  即 a 是指针，b 是 int，只有第一个是指针！

typedef int* pInt_t;
pInt_t a, b;    // a 和 b 都是 int* 指针
```
