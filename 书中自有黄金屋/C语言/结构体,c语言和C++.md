# 结构体, C语言和C++

[← C语言知识地图](./MOC.md)

> [结构体基础(csdn)](https://blog.csdn.net/m0_51064412/article/details/130352792)

---

## 内存对齐

结构体会内存对齐,导致 *实际实际占用内存空间*>=*结构体成员占用相加*

 **结构体对齐规则** （以默认对齐为例）：

* 每个成员的偏移量必须是**该成员大小和对齐值中较小值的整数倍**
* 结构体总大小必须是**最大成员对齐值的整数倍**
* 嵌套结构体按其**最大成员对齐值**对齐

## 修改内存对齐

* `#pragma pack(n)`：设置n字节对齐
* `__attribute__((packed))`：取消对齐，紧凑排列
* `__attribute__((aligned(n)))`：指定最小对齐值

```c
typedef struct __attribute__((packed)) {
    unsigned char head;
    unsigned short len;
    unsigned char cmd;
} Packet;
```

`packed` 的作用是：**尽量取消编译器自动插入的填充字节**，让结构体布局更贴近字节流本身。

在"*通信协议,寄存器映射,文件格式解析,网络报文""*等场景常见
