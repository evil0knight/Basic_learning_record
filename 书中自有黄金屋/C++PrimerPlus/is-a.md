# is-a 关系（第 13 章）

[← 返回虚函数](虚函数.md) | [← 主页](../../../README.md)

---

## 目录

- [is-a 的含义](#is-a-的含义)
- [访问权限](#访问权限)
- [is-a vs has-a vs is-implemented-as](#is-a-vs-has-a-vs-is-implemented-as)
- [派生类不继承的内容](#派生类不继承的内容)

---

## 🔴is-a 的含义

🔺**is-a** 是公有继承的语义：派生类对象**也是**一个基类对象，可以在任何需要基类的地方使用派生类

```cpp
class Animal {
public:
    void eat();
};

class Dog : public Animal {  // Dog is-a Animal
    void bark();
};

Animal* p = new Dog();  // ✅ 合法：Dog is-a Animal
```

🔺公有继承建立的是 is-a 关系，意味着：
- 派生类对象可以赋值给基类指针/引用
- 派生类对象可以调用基类的 public 方法
- 基类能做的事，派生类都能做

---

## 🔴访问权限

🔺基类成员在派生类中的可见性：

| 基类成员    | 派生类内部能否访问 | 外部通过派生类能否访问 |
| ----------- | ------------------ | ---------------------- |
| public      | ✅ 能               | ✅ 能                   |
| protected   | ✅ 能               | ❌ 不能                 |
| private     | ❌ 不能             | ❌ 不能                 |

🔺`protected` 的意义：专门为派生类开放，外部不可见

```cpp
class Base {
protected:
    int x;  // 派生类可访问，外部不可访问
};

class Derived : public Base {
public:
    void show() { cout << x; }  // ✅ 派生类内部可访问 protected
};

Derived d;
// d.x;  // ❌ 外部不能访问 protected
```

---

## 🔴is-a vs has-a vs is-implemented-as

🔺三种关系的选择：

| 关系                  | 实现方式 | 例子                         |
| --------------------- | -------- | ---------------------------- |
| is-a（是一种）        | 公有继承 | 午餐是一种食物               |
| has-a（有一个）       | 包含/组合 | 午餐有一个饮料               |
| is-implemented-as（用…实现） | 私有继承 | 用栈实现队列         |

```cpp
// is-a：公有继承
class Lunch : public Food {};

// has-a：组合（优先选这个）
class Lunch {
    Drink d;  // 午餐"有"一个饮料
};

// is-implemented-as：私有继承
class Queue : private Stack {};  // 用栈的功能实现队列，但不暴露栈的接口
```

🔺**原则**：能用组合就不用继承；只有真正存在 is-a 关系时才用公有继承

---

## 🔴派生类不继承的内容

🔺以下内容**不会**被继承：

- 构造函数
- 析构函数
- 赋值运算符 `operator=`
- 友元函数（不是成员函数）

🔺派生类需要自己定义这些，或者显式调用基类版本

---

## 本节小结

| 要点             | 说明                                           |
| ---------------- | ---------------------------------------------- |
| is-a 语义        | 派生类对象可用于任何需要基类的场合             |
| protected        | 专为派生类开放，外部不可见                     |
| 优先组合         | has-a 关系用组合，不要滥用继承                 |
| 不继承的内容     | 构造函数、析构函数、赋值运算符、友元           |
