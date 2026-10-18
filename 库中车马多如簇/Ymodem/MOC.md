# Ymodem

[← 模块总览](../MOC.md) | [← 主页](../../index.md)

[← 返回 OTA 升级](../../术中自有万钟粟/OTA/MOC.md)

## 文件

- [ymodem.h](./ymodem.h)
- [ymodem.c](./ymodem.c)
- [common.h](./common.h)
- [common.c](./common.c)

## 输入输出

此模块用在数据链路层传输文件

是接收端使用这个模块,当运行到这个模块的主要:`Ymodem_Receive`函数的时候:,内部发生:

1. Ymodem_Receive (uint8_t *buf):
   1. Send_Byte (uint8_t c)
      1. SerialPutChar(uint8_t c):这里改发送方式
   2. Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
      1. Receive_Byte (uint8_t *c, uint32_t timeout):轮询检测有没有接收到信息,然后放到 *c里
   3. FLASH_PagesMask(size):计算需要擦除多少页
   4. FLASH_ErasePage():标准外设库提供的flash页擦除函数
   5. FLASH_ProgramWord():标准外设库里的写入函数(这里应该直接把flashblock放到flash函数里去)

可以合理改代码,来适配其他比如:

DMA接收,
使用wrapper层的flash去解耦底层具体flash操作,
不应该直接擦写flash,而是应该用ABram,内存池,缓存起来然后去擦除flash
目前未动
