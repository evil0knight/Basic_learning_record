# 备份+AES 加密 — OTA 固件保护

[← OTA](./MOC.md) | [← 主页](../../index.md)

> AES-128-CBC：OTA 场景最常用的对称加密方案，128 位密钥 + 16 字节 IV，固件按 16 字节分块加密。
>
> 意义:防止升级失败导致系统变砖,代码回滚
>
> 1. 新固件通过OTA加密下载到非活动区
> 2. *验证新固件的完整性和有效性（如CRC校验、AES解密）*
> 3. 若验证通过，数据解密后搬运到活动区

---

## 备份:

[Ymodem传输协议](./Ymodem传输协议.md)的基础上,做出如下修改:

1. 在 `Boot_Manager.h`新增

   ```c
   /* APP_BACK：0x08020000~0x0803FFFF，占用 Sector 5，共 128 KiB。 */
   #define APP_BACK_START_ADDRESS  APP_RUN_END_ADDRESS
   #define APP_BACK_END_ADDRESS    0x08040000U
   #define APP_BACK_SIZE           (APP_BACK_END_ADDRESS - APP_BACK_START_ADDRESS)
   ```

   新增声明原型：`int8_t BackToApp(uint32_t app_size);`并包含头文件 `#include <stdint.h>`
2. `ymodem.c` 需要 `Ctrl+F`查找并 `APP_RUN_START_ADDRESS`替换 `APP_BACK_START_ADDRESS`

   > 接收文件大小仍使用 *APP_RUN_SIZE* 限制。*APP_BACK* 虽占用 128 KiB，但固件解密后必须能放入 96 KiB 的 *APP_RUN*。
   >
3. 在 `Boot_Manager.c` 新增 `BackToApp()`

   ```c
   int8_t BackToApp(uint32_t app_size)//把备份区已解密的固件搬到运行区
   {
       uint32_t src, dst, i;

       if (app_size == 0U || app_size > APP_RUN_SIZE)
       {
           return -1;
       }

       /* 先擦运行区，防止旧固件残留导致写入失败 */
       if (Flash_erase(APP_RUN_START_ADDRESS, APP_RUN_SIZE) != 0U)
       {
           return -2;
       }

       src = APP_BACK_START_ADDRESS;
       dst = APP_RUN_START_ADDRESS;

       for (i = 0; i < app_size; i += 4U)
       {
           Flash_Write(dst, *(uint32_t *)src);

           if (*(uint32_t *)dst != *(uint32_t *)src)
           {
               return -3; /* 写回校验失败 */
           }

           src += 4U;
           dst += 4U;
       }

       return 0;
   }
   ```

   顶部 `#include "flash.h"`（`Flash_erase` / `Flash_Write` 在里面）。

## AES加密

在「备份」流程上再套一层：上位机先把固件 AES-128-CBC 加密，Bootloader 收到密文后**解密再搬运到运行区**，防止串口明文传输被截获或篡改。

`库中车马多如簇/AES`中两个文件 `AES.c` / `AES.h`

| 函数                                         | 作用                  |
| -------------------------------------------- | --------------------- |
| `Aes_IV_key128bit_Encrypt(IV, State, key)` | 128bit 加密 1 块      |
| `Aes_IV_key128bit_Decode(IV, State, key)`  | 128bit 解密 1 块      |
| `Aes_IV_key192bit_Encrypt / Decode`        | 192bit 加 / 解密 1 块 |
| `Aes_IV_key256bit_Encrypt / Decode`        | 256bit 加 / 解密 1 块 |

三个形参（加解密语义相反，见 `AES.h` 注释）：

| 参数             | 加密                  | 解密                   |
| ---------------- | --------------------- | ---------------------- |
| `IV_IN_OUT`    | 向量输入 → 密文输出  | 向量输入 → 原密文输出 |
| `State_IN_OUT` | 明文输入 → 密文输出  | 密文输入 → 明文输出   |
| `key`          | 16 / 24 / 32 字节密钥 | 16 / 24 / 32 字节密钥  |

**CBC 链式靠 `IV_IN_OUT` 自动完成**：函数内部会把本块密文写回 `IV`，调用者只需传同一个 `IV` 缓冲区循环调用，即可连续解完整段固件，无需手动维护前一块密文。

源码注意（`AES.c`）：

- 每次调用都重新 `Aes_Key_Schedule_Create()` 生成轮密钥——OTA 一次性解密无碍，高频加解密应改为只生成一次。
- `key == NULL` 直接返回；`IV_IN_OUT == NULL` 退化为 ECB（不异或向量），OTA 必须传 IV 走 CBC。

### 上位机：串口加密传输工具

工具：`STM32串口ISP及文件加密工具.exe`，位置 `E:\ProgramFile\UART_Serial_Assistant\串口加密传输工具\`，[下载链接](https://pan.baidu.com/s/1McQ1cT1JvEs1lzYHR_EppQ?pwd=4444)(配置学习里有)

### Bootloader：解密搬运

新写一个解密搬运函数，替代「备份」节的 `BackToApp()`（也可两个都保留）。

1. `Boot_Manager.h` 新增原型：

   ```c
   int8_t BackToApp_AES(uint32_t app_size);
   ```

2. `Boot_Manager.c` 顶部，`#include "flash.h"` 后新增：

   ```c
   #include "aes.h"
   #include <string.h>

   /* 与上位机加密工具保持一致 */
   static const uint8_t AES_KEY[16] = { 0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
                                        0x39,0x30,0x31,0x32,0x33,0x34,0x35,0x36 };
   static const uint8_t AES_IV[16]  = { 0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,
                                        0x69,0x70,0x71,0x72,0x73,0x74,0x75,0x76 };
   ```

3. `Boot_Manager.c` 新增函数：

   ```c
   int8_t BackToApp_AES(uint32_t app_size)   /* app_size 需为 16 的整数倍 */
   {
       uint32_t i, j;
       uint8_t  iv[16];
       uint32_t block[4];

       if (app_size == 0U || app_size > APP_RUN_SIZE || (app_size % 16U) != 0U)
       {
           return -1;
       }

       if (Flash_erase(APP_RUN_START_ADDRESS, APP_RUN_SIZE) != 0U)
       {
           return -2;
       }

       memcpy(iv, AES_IV, 16);   /* 复位 CBC 向量 */

       for (i = 0; i < app_size; i += 16U)
       {
           memcpy(block, (void *)(APP_BACK_START_ADDRESS + i), 16);   /* 读密文 */
           Aes_IV_key128bit_Decode(iv, (uint8_t *)block, (uint8_t *)AES_KEY);    /* 解密，iv 链式更新 */

           for (j = 0; j < 4; j++)
           {
               Flash_Write(APP_RUN_START_ADDRESS + i + (j * 4U), block[j]);

               if (*(uint32_t *)(APP_RUN_START_ADDRESS + i + (j * 4U)) != block[j])
               {
                   return -3; /* 写回校验失败 */
               }
           }
       }

       return 0;
   }
   ```

4. Keil 工程把 `AES.c` 加入工程、`AES.h` 目录加入 Include Paths。
