# AES 加解密

| [← 模块总览](../MOC.md) | [← 主页](../../index.md) |
| --- | --- |

纯 C 实现的 AES 对称加解密，一次处理 16 字节一个数据块，支持 128 / 192 / 256 bit 三种密钥；CBC 链式由调用者通过 `IV` 缓冲区维持。

| 文件 | 内容 |
| --- | --- |
| [AES.h](./AES.h) | 加解密接口声明 |
| [AES.c](./AES.c) | 加解密实现（S 盒 / 密钥扩展 / 列混淆等） |
| [OTA AES 加密](../../术中自有万钟粟/OTA/AES加密.md) | OTA 场景的使用与解密搬运 |

## 接口

| 函数 | 作用 |
| --- | --- |
| `Aes_IV_key128bit_Encrypt / Decode` | 128bit 加 / 解密 1 块 |
| `Aes_IV_key192bit_Encrypt / Decode` | 192bit 加 / 解密 1 块 |
| `Aes_IV_key256bit_Encrypt / Decode` | 256bit 加 / 解密 1 块 |

形参：`IV_IN_OUT`（向量，跨块链式）、`State_IN_OUT`（明文 / 密文块）、`key`（16 / 24 / 32 字节密钥）。

## 使用要点

- CBC 链式靠 `IV_IN_OUT` 自动完成：循环传同一个 `IV` 缓冲区即可连续解密整段数据。
- 每次调用都重新生成轮密钥（`Aes_Key_Schedule_Create`），高频调用应改为只生成一次。
- `IV_IN_OUT == NULL` 退化为 ECB（不异或向量）；`key == NULL` 直接返回。
