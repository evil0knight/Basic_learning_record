# Ymodem 传输协议

[← OTA](./MOC.md) | [← 主页](../../index.md)

> [CSDN 详解](https://blog.csdn.net/huangdenan/article/details/103611081) | [立芯协议讲解](https://twd6onxsxva.feishu.cn/docx/QFGNdILHSoTBW2xdgWRcKrmnnAe?from=from_parent_docx) | [立芯移植视频](https://twd6onxsxva.feishu.cn/docx/U8EUdres3oy4LuxLbxqckmaVnOd)|
>
> [Ymodem 源码与适配文件](../../库中车马多如簇/Ymodem/MOC.md)

Ymodem 只负责可靠地把文件从上位机传到单片机。串口初始化、Flash 擦写、升级入口、固件合法性检查和跳转 APP 都由 Bootloader 完成。

---

[Ymodem 源码与适配文件](../../库中车马多如簇/Ymodem/MOC.md)

### 复用 Boot Manager

Keil 的 Bootloader/APP 地址、向量表和 Boot Manager 配置见 [IAP 实操 + 原理](./IAP实操+原理.md)。

Ymodem 还需将 `common.c`、`ymodem.c`、`flash.c`、`Usart.c` 加入 Keil 工程，并将对应头文件目录加入 **C/C++ → Include Paths**。

## 文件代码编写:

1. `common.h`：只保留协议使用的 `PAGE_SIZE`；固件大小统一使用 `APP_RUN_SIZE`，不再重复定义大小宏。

   ```c
   #define PAGE_SIZE  (0x800U)
   ```
2. 调用 `Ymodem_Receive()` 前初始化需要的通信接口。当前 `Usart.c` 默认初始化 USART1；使用其他串口时初始化对应 USART。
3. 接收接口：`ymodem.c` 第 58 行调用 `SerialKeyPressed()`，在 `common.c` 第 201 行实现。

   ```c
   uint32_t SerialKeyPressed(uint8_t *key)
   {
       if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
       {
           *key = (uint8_t)USART1->DR;
           return 1U;
       }
       return 0U;
   }
   ```

   使用 USART2 时，将两处 `USART1` 改为 `USART2`。该函数必须立即返回：收到数据返回 `1`，没有数据返回 `0`，不能直接调用阻塞等待的 `USART_ReceiveChar()`。
4. 发送接口：`ymodem.c` 第 73 行调用 `SerialPutChar()`，在 `common.c` 第 246 行实现。

   ```c
   void SerialPutChar(uint8_t c)
   {
       USART_SendChar(USART1, c);
   }
   ```

   使用 USART2 时，将 `USART1` 改为 `USART2`。使用 USB CDC 等其他字节流接口时，只替换 `SerialKeyPressed()` 和 `SerialPutChar()` 的函数体，不修改 `ymodem.c`。
5. 擦除接口：`ymodem.c` 第 219 行调用 `Flash_erase(APP_RUN_START_ADDRESS, size)`，对应 `flash.c` 第 57 行。接口返回 `0` 表示成功，返回 `1` 表示失败。
6. 写入接口：`ymodem.c` 第 247 行调用 `Flash_Write(FlashDestination, data)`，对应 `flash.c` 第 116 行。当前协议代码每次写入 4 字节，并在写入后回读比较。
7. 芯片配置：

   - `flash.h` 第 34~45 行的 `ADDR_FLASH_SECTOR_x` 来自芯片 Flash 扇区分布，用于把写入地址换算为 SPL 的 `FLASH_Sector_x`。扇区边界必须按目标芯片数据手册配置。
   - `flash.c` 第 111 行的 `FLASH_EraseSector()` 和 `VoltageRange_3` 来自 STM32F4 标准外设库：声明和宏定义位于 `stm32f4xx_flash.h`，函数实现在 `stm32f4xx_flash.c`。本工程通过 `flash.h -> stm32f4xx.h -> stm32f4xx_conf.h` 间接包含该头文件。
   - `VoltageRange_3` 表示芯片供电电压为 $2.7\text{ V}\sim3.6\text{ V}$，适用于常见的 3.3 V 供电。它不会设置芯片电压，而是让 `FLASH_EraseSector()` 按该电压范围设置 Flash 控制寄存器的编程并行度 `PSIZE`；电压越高，可安全并行操作的数据宽度越大。范围选择错误可能导致擦除失败或操作不可靠。

   更换芯片时，按目标芯片数据手册修改扇区地址和供电电压范围；若仍使用 STM32F4 SPL，不修改 `ymodem.c`。
8. APP_RUN 配置：`APP_RUN_START_ADDRESS` 是擦除、写入和跳转起点，`APP_RUN_SIZE` 是允许接收的最大文件大小（96 KiB），使用 [IAP 实操 + 原理](./IAP实操+原理.md)中 `Boot_Manager.h` 的配置。

Ymodem 只调用 `Flash_erase()` 和 `Flash_Write()` 两个 Flash 接口。

## 补接收 CRC 校验(可写可不写)

### `ymodem.c`：第 91 行

```c
uint16_t i, packet_size, received_crc, calculated_crc;
```

### `ymodem.c`：第 136 行后插入

```c
received_crc = ((uint16_t)data[PACKET_HEADER + packet_size] << 8)
             | data[PACKET_HEADER + packet_size + 1U];
calculated_crc = Cal_CRC16(data + PACKET_HEADER, packet_size);

if (received_crc != calculated_crc)
{
    return -1;
}
```

## 调用接收函数

### Bootloader `main.c`：加入接收与跳转代码

`uint32_t ymodem_buffer[256]` 是 1024 字节、4 字节对齐的接收缓冲区

```c
#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "elog.h"
#include "stm32f4xx.h"
#include "ymodem.h"
#include "Boot_Manager.h"
#include "USART.h"

static uint32_t ymodem_buffer[256];

/* 示例：上电按键触发升级，也可替换为升级标志或串口命令。 */
int UpgradeRequested(void)
{
    return Key_Scan() ? 1 : 0;
}
```

`DisablePeripherals()` 和 `JumpToApp()` 已在 [IAP 实操 + 原理](./IAP实操+原理.md)的 `Boot_Manager.c` 中实现，这里只需包含 `Boot_Manager.h` 后调用。

```c
int main(void)
{
    int32_t received_size;

    RCC_ClockSecuritySystemCmd(ENABLE);
    SCB->VTOR = BOOTLOADER_START_ADDRESS;

    SystemCoreClockUpdate();
    RCC_GetClocksFreq(&RCC_Clocks);
    SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

    Delay(50);

    GPIO_Config();
    TIM_Config();
    USART1_Init();
    app_elog_init();

    /* 由按键、升级标志或上位机命令决定是否进入升级。 */
    if (UpgradeRequested())
    {
        received_size = Ymodem_Receive((uint8_t *)ymodem_buffer);

        if (received_size > 0)
        {
            DisablePeripherals();
            NVIC_SystemReset();
        }

        /* 接收失败：保留在 Bootloader，等待重试，不能跳入半包 APP。 */
        while (1)
        {
        }
    }

    DisablePeripherals();
    JumpToApp();

    while (1)
    {
        /* 无有效 APP：等待进入升级模式。 */
    }
}
```

`UpgradeRequested()` 是项目自己的升级入口，可由以下任一种方式实现：

| 触发方式                              | 场景                       |
| ------------------------------------- | -------------------------- |
| 上电按住按键                          | 调试、救砖                 |
| APP 写入备份寄存器或 Flash 标志后复位 | 正常在线升级               |
| Bootloader 短时间等待串口命令         | 无按键设备                 |
| APP 向量表无效                        | 首次烧录或升级中断后的恢复 |

建议升级成功后执行 `NVIC_SystemReset()`，让 APP 从干净的复位状态启动。直接跳转时必须额外关闭中断、SysTick、DMA 和 Bootloader 使用过的外设，并清除 NVIC pending 位。

APP 的 `main()` 在初始化外设前恢复向量表和全局中断：

```c
int main(void)
{
    SCB->VTOR = APP_RUN_START_ADDRESS;
    __enable_irq();

    /* APP 自己的时钟和外设初始化。 */
    while (1)
    {
    }
}
```

## 返回值

当前参考实现的返回值并不完全区分所有失败原因：

| 返回值  | 含义                               |
| ------- | ---------------------------------- |
| `> 0` | 接收成功，值为文件实际字节数       |
| `-1`  | 文件过大或擦除失败                 |
| `-2`  | Flash 写入后回读不一致             |
| `-3`  | 用户或发送端主动取消               |
| `0`   | 空会话、发送端取消，或重试次数耗尽 |

只有“返回值大于 0、APP 向量表合法、可选的整包校验通过”三个条件同时成立，才允许启动新 APP。

## SecureCRT 上位机发送

安装教程已经放到[配置学习](../配置学习/MOC.md)里了,软件位置在 `E:\ProgramFile\UART_Serial_Assistant\SecureCRT\open_exe`

### 发送 APP

1. Keil → 魔法棒 → Output → 勾选 Create HEX File；再切到 User → After Build 勾选 Run #1，填入：
   $K\ARM\ARMCC\bin\fromelf.exe --bin --output=@L.bin !L
   编译后 `.bin` 在 `.axf` 同目录（通常是 `Objects/`）,**注意,是APP的bin,别找错了💦**
2. 复位板卡并触发升级模式，等待 SecureCRT 终端连续出现 `C`。这表示程序已进入 `Ymodem_Receive()` 并请求 CRC16 传输。
3. 点击 `Transfer → Send Ymodem...`，选择 APP 的 `.bin`，确认后开始传输。
4. SecureCRT 弹出传输进度窗口后，Bootloader 依次接收文件信息包和数据包；不要在终端中继续输入字符。
5. 进度达到 100% 且窗口正常结束后，Bootloader 校验 APP 向量表并复位，APP 从 `0x08008000` 启动。

必须先让板卡进入接收模式并出现 `C`，再点击 `Send Ymodem`。如果 SecureCRT 一直停在等待状态，先取消发送，确认终端能看到 `C` 后重试。
