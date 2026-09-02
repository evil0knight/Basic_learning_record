# MD5 摘要

[← 模块总览](../MOC.md)

MD5 散列算法（纯软件实现），输入任意长度数据输出固定 16 字节摘要。OTA 中用于固件完整性校验。

| 文件 | 内容 |
| --- | --- |
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

OTA 中用法见 [OTA](../OTA/MOC.md) 的 BootLoader 端：解密后对应用数据算 MD5，与固件尾部带的 16 字节 MD5 比对，一致才判定升级成功。
