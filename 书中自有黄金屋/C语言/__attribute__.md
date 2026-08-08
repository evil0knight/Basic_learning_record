# __attribute__

[← C语言知识地图](./MOC.md)

---

**核心：GCC 编译器扩展，给变量/函数/类型附加属性，控制编译行为。**

## 常用属性速查

| 属性                | 作用                                                                  | 嵌入式常用 |
| ------------------- | --------------------------------------------------------------------- | :--------: |
| `packed`          | 取消[结构体](./结构体,c语言和C++.md)对齐，紧凑排布                       |     ✅     |
| `aligned(n)`      | 指定对齐字节数                                                        |            |
| `section("name")` | [section 划分区域](../../术中自有万钟粟/编译链接下载/section划分区域.md) |     ✅     |
| `weak`            | 弱符号，可被同名强符号覆盖                                            |     ✅     |
| `unused`          | 标记未使用，消除编译警告                                              |            |
| `deprecated`      | 标记弃用，调用时产生警告                                              |            |
| `constructor`     | 在 `main()` 之前自动执行                                            |            |
| `destructor`      | 在 `main()` 之后 / `exit()` 时执行                                |            |

## 示例

### packed — 紧凑排列

```c
struct __attribute__((packed)) {
    char  a;      // 1 byte
    int   b;      // 4 bytes — 不加 packed 会因对齐插入 3 字节 padding
    short c;      // 2 bytes
};
// sizeof = 7（无 padding），而非 12
```

### section — 放入指定段

详见 [section 划分区域](../../术中自有万钟粟/编译链接下载/section划分区域.md)

```c
int boot_flag __attribute__((section(".noinit"))) = 0;  // 上电不初始化
void __attribute__((section(".ramfunc"))) critical_isr(void) { }
```

常用于：noinit（复位不丢失）、ramfunc（Flash 代码搬运到 RAM 执行）。

### weak — 弱符号

```c
void __attribute__((weak)) HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    // 默认空实现，用户在别处定义同名函数即可覆盖
}
```

HAL 库大量使用：提供默认弱回调，用户只需在同名强函数里实现自己的逻辑。

### unused / deprecated

```c
int old_func(void) __attribute__((deprecated));       // 调用者收到弃用警告
void isr_handler(void) {
    int ret __attribute__((unused)) = some_call();    // ret 不使用也不报 warning
}
```

### constructor / destructor

```c
void __attribute__((constructor)) before_main(void) {
    // main 之前执行——注意此时 C 运行时尚未完全就绪
}
void __attribute__((destructor)) after_main(void) {
    // main 结束后执行
}
```
