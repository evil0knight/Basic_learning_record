# virtual 关键字与多态（第 13 章）

[← 返回虚函数](虚函数.md) | [← 主页](../../../README.md)

---

## 目录

- [静态联编 vs 动态联编](#静态联编-vs-动态联编)
- [virtual 的用法](#virtual-的用法)
- [虚函数表 vtable](#虚函数表-vtable)
- [虚析构函数](#虚析构函数)
- [virtual 的限制](#virtual-的限制)

---

## 🔴静态联编 vs 动态联编

🔺**联编**：将函数调用与具体函数体绑定的过程

🔺**静态联编**（无 virtual）：编译期根据**指针/引用的类型**决定调用哪个函数

```cpp
class Animal {
public:
    void speak() { cout << "...\n"; }
};
class Dog : public Animal {
public:
    void speak() { cout << "Woof\n"; }
};

Animal* p = new Dog();
p->speak();  // 输出 "..."  ← 看指针类型 Animal*，调用 Animal::speak
```

🔺**动态联编**（有 virtual）：运行期根据**对象的实际类型**决定调用哪个函数

```cpp
class Animal {
public:
    virtual void speak() { cout << "...\n"; }
};
class Dog : public Animal {
public:
    virtual void speak() { cout << "Woof\n"; }
};

Animal* p = new Dog();
p->speak();  // 输出 "Woof" ← 看对象实际类型 Dog，调用 Dog::speak
```

---

## 🔴virtual 的用法

🔺在基类函数声明前加 `virtual`，派生类中该函数自动也是虚函数（不需要重复写，但建议加 `override`）

```cpp
class Base {
public:
    virtual void func();   // 声明为虚函数
};

class Derived : public Base {
public:
    void func() override;  // override 让编译器检查是否真的重写了基类虚函数
};
```

🔺`override` 的作用：若函数签名写错(原基类无重写的这个函数)，编译器会报错，避免静默地创建新函数而非重写

```cpp
class Derived : public Base {
public:
    void func(int x) override;  // ❌ 编译错误：基类没有 func(int)，不是重写
};
```

🔺**重写（override）的条件**：函数名、参数列表、const 属性必须完全一致，返回类型通常也要一致（协变返回类型除外）

---

## 🔴虚函数表 vtable

🔺每个含虚函数的类有一张**虚函数表（vtable）**，每个对象有一个隐藏的**虚指针（vptr）**指向该表

```
Dog 对象内存布局：
┌─────────┐
│  vptr   │ ──→ Dog::vtable ──→ [ &Dog::speak, &Dog::eat, ... ]
├─────────┤
│  数据   │
└─────────┘

Animal* p = new Dog();
p->speak();
// 运行时：通过 vptr 找到 Dog::vtable，再找到 Dog::speak 并调用
```

🔺**开销**：每个对象多一个指针大小（通常 8 字节）；每次虚函数调用多一次间接寻址

---

## 🔴虚析构函数

🔺通过基类指针 `delete` 派生类对象时，若析构函数**不是虚函数**，只调用基类析构函数 → 派生类资源泄漏

```cpp
class Base {
public:
    ~Base() { cout << "Base 析构\n"; }  // 非虚
};
class Derived : public Base {
    int* data;
public:
    Derived() { data = new int[100]; }
    ~Derived() { delete[] data; }       // 不会被调用！
};

Base* p = new Derived();
delete p;  // 只调用 ~Base()，~Derived() 未调用 → 内存泄漏
```

🔺**解决**：基类析构函数声明为 `virtual`

```cpp
class Base {
public:
    virtual ~Base() {}  // ⭐ 只要类会被继承，析构函数就应该是虚函数
};
```

---

## 🔴virtual 的限制

🔺**构造函数不能是虚函数**：构造时对象还未完全建立，vptr 尚未初始化，虚机制无法工作

🔺**静态成员函数不能是虚函数**：静态函数没有 `this` 指针，不属于任何对象实例

🔺**友元函数不能是虚函数**：友元不是成员函数

🔺**inline 函数声明为 virtual 通常无意义**：虚函数在运行期动态绑定，inline 在编译期展开，两者矛盾

---

## 本节小结

| 要点         | 说明                                             |
| ------------ | ------------------------------------------------ |
| 静态联编     | 无 virtual，编译期按指针类型绑定                 |
| 动态联编     | 有 virtual，运行期按对象实际类型绑定             |
| override     | 让编译器验证重写是否正确                         |
| vtable/vptr  | 每个含虚函数的类有虚表，每个对象有虚指针         |
| 虚析构函数   | 基类析构必须是 virtual，否则派生类析构不会被调用 |
| 不能是虚函数 | 构造函数、静态成员函数、友元函数                 |
