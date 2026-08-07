# assert — 断言

[← C语言知识地图](./MOC.md)

---

**核心：运行时检查条件，条件为假则终止程序，用于捕捉逻辑 Bug。**

## 基本用法

```c
#include <assert.h>

void set_age(int age) {
    assert(age >= 0);                    // 年龄不可能为负，否则是 Bug
    printf("Setting age to %d\n", age);
}

int main() {
    set_age(-5);                         // 断言失败 → 程序终止
    return 0;
}
```

- 断言失败时打印 `文件名:行号: 函数名: Assertion '条件' failed.` 并调用 `abort()` 终止程序
- 仅用于开发调试，不应替代正常的错误处理（如参数校验返回错误码）

## 生产环境关闭

```c
#define NDEBUG                          // 在所有 #include <assert.h> 之前定义
#include <assert.h>                     // 此后 assert() 变为空操作
```

`NDEBUG` 定义后所有 `assert()` 宏展开为空，无运行时开销。通常在 Release 构建时由编译器隐式定义（如 `-DNDEBUG`）。
