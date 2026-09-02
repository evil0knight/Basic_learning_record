# 串口printf

[← 返回 MOC](./MOC.md) | [← 主页](../../../index.md)

---

## 1打开[microlib](../../../术中自有万钟粟/嵌软高手/microlib打开原因.md)

## 2配置串口通信和GPIO

在usart.c中的begin和end之间写入如下代码

```
#include "usart.h"

/* USER CODE BEGIN 0 */
#include <stdio.h> //包含头文件

 #ifdef __GNUC__
     #define PUTCHAR_PROTOTYPE int _io_putchar(int ch)
 #else
     #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
 #endif /* __GNUC__*/
 
 /******************************************************************
     *@brief  Retargets the C library printf  function to the USART.
     *@param  None
     *@retval None
 ******************************************************************/
 PUTCHAR_PROTOTYPE
 {
     HAL_UART_Transmit(&huart4, (uint8_t *)&ch,1,0xFFFF);
     return ch;
 }
```

## 3main.h中:

包含`#include	<stdio.h>`
