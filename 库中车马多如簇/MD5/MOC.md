# MD5 摘要

[← 模块总览](../MOC.md) | [← 主页](../../index.md)

[← 返回 OTA MOC](../../术中自有万钟粟/OTA/MOC.md)

---

MD5 散列算法（纯软件实现），输入任意长度数据输出固定 16 字节摘要。

和CRC都是用来校验有没有传输错误的,不过这个更严密

| 文件                          | 内容         |
| ----------------------------- | ------------ |
| [md5.h](./md5.h) / [md5.c](./md5.c) | MD5 摘要算法 |

## 移植

1. 复制 `md5.h`、`md5.c` 到工程的 `Middleware/MD5/` 目录，将 `md5.c` 加入编译。
2. 需要校验的业务文件里 `#include "md5.h"`。
3. MD5 不依赖 HAL / OS / 硬件，不需要 Port / Wrapper / 注册表。

## 接口调用

```c
#include "md5.h"
#include <string.h>

uint8_t data[1024];
uint8_t digest[16];
MD5_CTX ctx;

MD5Init(&ctx);
MD5Update(&ctx, data, sizeof(data));   /* 可分多次喂数据 */
MD5Final(&ctx, digest);                /* 得到 16 字节摘要 */
```
