# C++ Primer Plus — 知识地图

[← 学科总览](../MOC.md) | [← 主页](../../README.md)

> 状态：正在啃,1~9速通,后面刷题精学LeetCode

---

![1774354468251](image/MOC/1774354468251.png)

<details>
<summary><h3 style="display:inline">🔴第 1 章 预备知识</h3></summary>

C已经学了

</details>

<details>
<summary><h3 style="display:inline">🔴第 2 章 开始学习 C++</h3></summary>

🔺用 `int main()` 不用 `void main()`：**void main** 逻辑上没问题，但不是标准强制要求，部分系统**不支持**

🔺使用 C++ 输入输出必须写 `#include <iostream>`，io = input/output

🔺**C++ 头文件无扩展名**；C 头文件转 C++ 版本时去掉 `.h` 并加前缀 `c`，如 `math.h` → `cmath`

🔺`using namespace std`; 让你直接写 `cout` 而不用写 `std::cout`,后面会讲

🔺`cout << "text" << endl`;：`<<` 是插入运算符**,cout是一个预定义的对象知道如何显示字符串**,数字,单个字符等(cout<<string;)

🔺`endl` **输出换行并刷新缓冲区**（`\n` 只换行不刷新）

🔺在**C语言中,变量需要再函数或者过程的开头声明**,而C++不需要,通常在第一次使用的时候声明

🔺cout会**自动判断类型,**不像C语言要用%d,%s,%f进行区分,这是因为**运算符<<重载**,后面会讲

🔺cin也一样,它可以通过键盘输入的一系列字符(即输入)转换为接受信息的变量能够接受的形式

<details>
<summary><style="display:inline">🔺类简介</summary>

类是用户定义的一种数据类型。要定义类，**需要描述它能够表示什么信息和可对数据执行哪些操作**。类之于对象就像类型之于变量。也就是说，类定义描述的是数据格式及其用法，而对象则是根据数据格式规范创建的实体。

现在来看 **cout,它是一个 ostream 类对象**。ostream 类定义（iostream 文件的另一个成员）**描述了 ostream 对象表示的数据以及可以对它执行的操作**，如将数字或字符串插入到输出流中。同样，cin 是一个 istream 类对象，也是在 iostream 中定义的。

</details>

<details>
<summary><style="display:inline">🔺使用命名空间中的函数</summary>

总之，让程序能够访问名称空间 std 的方法有多种，下面是其中的 4 种。

* 将 `using namespace std;` 放在**函数定义之前**，让文件中**所有的函数都能够使用**名称空间 std 中所有的元素。
* 将 `using namespace std;` 放在**特定的函数定义中**，让该**函数能够使用**名称空间 std 中的所有元素。
* 在特定的函数中使用类似 `using std::cout;` 这样的编译指令，而不是 `using namespace std;`，让**该函数能够使用指定的元素**，如 cout。
* 完全不使用编译指令 `using`，而在需要使用名称空间 std 中的元素时，**使用前缀 `std::`**，如下所示：
  `std::cout << "I'm using cout and endl from the std namespace" << std::endl;`

</details>

</details>

<details>
<summary><h3 style="display:inline">🔴第 3 章 处理数据</h3></summary>


</details>

<details>
<summary><h3 style="display:inline">🔴第 4 章 复合类型</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 5 章 循环和关系表达式</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 6 章 分支语句和逻辑运算符</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 7 章 函数——C++ 的编程模块</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 8 章 函数探幽</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 9 章 内存模型和名称空间</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 10 章 对象和类</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 11 章 使用类</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 12 章 类和动态内存分配</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 13 章 类继承</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 14 章 代码重用</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 15 章 友元、异常和其他</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 16 章 string 类和标准模板库</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 17 章 输入、输出和文件</h3></summary>

</details>

<details>
<summary><h3 style="display:inline">🔴第 18 章 探讨 C++ 新标准</h3></summary>

</details>

---

## 高频考点
