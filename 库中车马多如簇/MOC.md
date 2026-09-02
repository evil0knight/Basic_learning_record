# 库中车马多如簇 — 模块知识地图

[← 返回主页](../index.md)|

> **记录各类硬件模块、芯片模块,工程架构的学习笔记**

---

## 开始一个工程,看看[嵌入式工程架构笔记](../术中自有万钟粟/嵌入式工程架构/MOC.md)

## 拿到一个模块,找4个资料:

1. 参考手册Reference Manual:怎么用芯片的，里面有详细寄存器描述、外设使用步骤、寄存器位说明，偏软件开发
2. 数据手册Datasheet:芯片规格说明书，告诉你电气参数、封装、时序、电源电流、温度范围，偏硬件选型和电路设计。
3. 原理图
4. 官方例程

## 模块入口

|      模块      |                      简介                      | 笔记                                                 |
| :-------------: | :--------------------------------------------: | ---------------------------------------------------- |
|    WiFi 模块    |                 涂鸦 WiFi 模组                 | [WiFi 模块笔记](./WiFi模块/WiFi模块.md)                 |
|    FT辉芒微    |                     skill                     | [FT辉芒微/MOC.md](FT辉芒微/MOC.md)                      |
|       LED       |                BSP分层驱动架构                | [BSP_LED](LED/BSP_LED/MOC.md)                           |
|      UART      |                    UART大全                    | [UART](../库中车马多如簇/UART/MOC.md) |
|       CAN       |                    CAN总线                    | [CAN](./CAN总线/MOC.md)                                 |
|  外设文件模板  |                   SKILL要用                   | [外设文件模板.md](外设文件模板.md)                      |
|    433 模块    |                  433MHz 无线                  | [433模块笔记](./433模块/MOC.md)                         |
|   硬件原器件   |                 实际用过的元件                 | [硬件原器件笔记](./硬件原器件/MOC.md)                   |
|      麦轮      |            麦克纳姆轮原理与底盘解算            | [麦轮笔记](./麦轮/MOC.md)                               |
|     超声波     |           HC-SR04 测距，输入捕获驱动           | [超声波笔记](./超声波/MOC.md)                           |
|     编码器     |         正交编码器测速，TIM 编码器模式         | [编码器笔记](./编码器/MOC.md)                           |
|     缓冲区     |        单缓冲,双缓冲,环形缓冲,乒乓缓冲        | [缓冲区笔记](缓冲区/MOC.md)                             |
|   EasyLogger   |         嵌入式日志库，源码 + keil 移植         | [使用方法](../术中自有万钟粟/debug方法/EasyLogger.md)   |
|      Flash      |        片内与外部 Flash 擦除、读写驱动        | [Flash](./Flash/MOC.md)                                 |
|     Ymodem     |         文件传输协议，源码 + 协议笔记         | [Ymodem](./Ymodem/MOC.md)                               |
|       AES       | 对称加密，128/192/256 bit CBC，源码 + OTA 使用 | [AES](./AES/MOC.md)                                     |
|       MD5       |          散列摘要，OTA 固件完整性校验           | [MD5](./MD5/MOC.md)                                     |
|       OTA       |       APP后台下载与BootLoader安装       | [OTA](./OTA/MOC.md)                                     |
|      看门狗      |       IWDG/WWDG统一Wrapper与Backend       | [看门狗](./看门狗/MOC.md)                                |
|       SPI       |          硬件SPI、软件SPI与统一Port          | [SPI](./SPI/MOC.md)                                     |
|       IIC       |       硬件I2C、软件I2C与统一总线Port        | [IIC](./IIC/MOC.md)                                     |
|     EEPROM      |             掉电保存与AT24C02驱动             | [EEPROM](./EEPROM/MOC.md)                               |
|      屏幕      |             LCD/OLED屏幕驱动与接入             | [屏幕](./屏幕/MOC.md)                                   |
|    代码架构    |        Wrapper 解耦与 Adapter 挂载源码        | [代码架构](./代码架构/MOC.md)                           |
|   外设库模板   |     照 Flash 抽象出的外设库骨架，写新外设照它    | [_模板](./_模板/MOC.md)                                 |
|  蓝牙bluetooth  |                     HC-05                     | [蓝牙bluetooth](蓝牙bluetooth\MOC.md)                   |

<!-- 整理已经熟悉的模块 -->
