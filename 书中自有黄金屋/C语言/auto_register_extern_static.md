# auto / register / extern / static

[← C语言知识地图](./MOC.md)

---

**核心：四个关键字控制变量的存储位置、生命周期和可见范围。**

| 关键字 | 存储位置 | 生命周期 | 作用域 | 关键特征 |
| ------ | -------- | -------- | ------ | -------- |
| `auto` | 栈 | 函数执行期间 | 块内 | 局部变量默认值，几乎不显式写 |
| `register` | CPU 寄存器（建议） | 函数执行期间 | 块内 | 不能取地址 `&`，编译器可能忽略 |
| `extern` | 全局数据段 | 整个程序运行期 | 跨文件 | 声明一个别处定义的变量/函数 |
| `static` | 全局数据段 | 整个程序运行期 | 文件内 / 块内 | 持久但不可跨文件访问 |

## auto

```c
void func(void) {
    auto int x = 1;  // 等价于 int x = 1;
}
```

- 默认就是 `auto`，写了纯属多余
- C 编译器唯一会**默认**加的存储类

## register

```c
register int counter;
for (counter = 0; counter < 1000000; counter++) { ... }
```

- 请求编译器将变量放入 **CPU 寄存器**，减少内存访问
- **不能取地址**：`&counter` 编译报错
- 编译器有权**忽略**该建议（现代编译器优化已很智能）

## extern

```c
// file1.c
int global = 42;

// file2.c
extern int global;   // 声明：告诉编译器 global 在别处定义
void func(void) {
    global = 100;    // 操作的是 file1.c 里的同一个变量
}
```

- **只声明不定义**，链接时解析
- 函数默认就是 `extern`，`extern void foo(void);` 与 `void foo(void);` 等价

## static

### 文件作用域（内部链接）

```c
static int counter = 0;  // 仅在当前 .c 文件内可见
```

- 其他文件同名变量互不冲突
- 全局变量应尽量用 `static` 限制可见性

### 函数内（持久局部变量）

```c
void count(void) {
    static int n = 0;  // 只初始化一次
    n++;
    printf("%d\n", n);
}
```

- 值在多次调用间**保持**
- 存储在全局数据段，生命周期 = 程序运行期

### 静态函数

```c
static void helper(void) { ... }  // 仅当前文件内可调用
```
