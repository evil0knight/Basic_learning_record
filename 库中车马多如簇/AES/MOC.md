# AES 加解密

[← 模块总览](../MOC.md) | [← 主页](../../index.md)
[← OTA AES加密](../../术中自有万钟粟/OTA/AES加密.md)

---

| 文件 | 内容 |
| --- | --- |
| [AES.h](./AES.h) / [AES.c](./AES.c) | AES-128/192/256加解密 |
| [OTA AES加密](../../术中自有万钟粟/OTA/AES加密.md) | OTA中Flash分块加解密流程 |

## 移植

1. 复制 `AES.h`、`AES.c` 到工程的 `Middleware/AES/` 目录，并将 `AES.c` 加入工程编译。
2. 在需要使用AES的业务文件中包含 `AES.h`。
3. 调用方需要提供：16/24/32字节密钥、16字节初始向量`IV`、以及16字节对齐的数据缓冲区。
4. 输入数据长度必须是16的整数倍，不足时由调用方补齐。
5. CBC连续处理时，所有数据块传入同一个`IV_IN_OUT`缓冲区；每次新消息重新复制初始IV。
6. AES不需要HAL、GPIO、UART、Flash适配函数，不新增Port、Wrapper或注册表。

## 接口调用

1. AES-128加密一个数据块：

```c
#include "AES.h"

uint8_t key[16] = {0};
uint8_t iv[16] = {0};
uint8_t block[16] = {0};

Aes_IV_key128bit_Encrypt(iv, block, key);
```

2. AES-128解密一个数据块：

```c
Aes_IV_key128bit_Decode(iv, block, key);
```

3. 多块CBC加密：

```c
#include "AES.h"

uint8_t key[16] = {0};
uint8_t iv[16] = {0};
uint8_t data[32] = {0};

Aes_key128bit_CBC_Encrypt(data, sizeof(data), iv, key);
```

4. 多块CBC解密：

```c
Aes_key128bit_CBC_Decode(data, sizeof(data), iv, key);
```

5. OTA场景中，从Flash读出数据后调用上述CBC接口，再将结果写回目标Flash；具体流程参考[OTA AES加密](../../术中自有万钟粟/OTA/AES加密.md)。
