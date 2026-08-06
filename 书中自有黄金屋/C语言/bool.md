# bool

[← C语言知识地图](./MOC.md)

---

**`bool` 实际占 8 位（1 字节），因为内存最小寻址单位是 8 位。**

```c
#include <stdbool.h>
bool flag = true;
printf("%zu\n", sizeof(flag));  // 输出 1，即 1 字节 = 8 位
```

- `_Bool` 是 C99 引入的内置类型，`stdbool.h` 将其 typedef 为 `bool`
- 即使只需 0/1 一个 bit，实际仍分配 1 字节——CPU 无法直接寻址单个 bit
- 嵌入式要用位域节省空间：
  ```
  struct SystemFlags {
      uint8_t is_ready    : 1; // 占 1 bit
      uint8_t is_error    : 1; // 占 1 bit
      uint8_t is_busy     : 1; // 占 1 bit
      uint8_t is_running  : 1; // 占 1 bit
      uint8_t wifi_ok     : 1; // 占 1 bit
      uint8_t ble_ok      : 1; // 占 1 bit
      uint8_t battery_low : 1; // 占 1 bit
      uint8_t tx_done     : 1; // 占 1 bit
  }; 
  // 8 个 bit 刚好拼成 1 个字节
  // sizeof(struct SystemFlags) = 只需要 1 字节！
  ```
