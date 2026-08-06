# const

[← C语言知识地图](./MOC.md)

---

**核心:靠近那边,那边就是不能变的**

## 1. `const int *p` — 指向常量的指针（pointer to const）

```
const int *p
```

从右往左读：**p is a pointer to an int that is const.**

- 改变的是 `int` 的属性 → **值不能变**
- `*p = 10;` ❌ 编译报错
- `p = &another;` ✅ 指针本身可以改指向

## 2. `int * const p` — 常量指针（const pointer）

```
int * const p
```

从右往左读：**p is a const pointer to an int.**

- 改变的是 pointer 的属性 → **指针本身不能变**
- `p = &another;` ❌ 编译报错
- `*p = 10;` ✅ 指向的值可以改
