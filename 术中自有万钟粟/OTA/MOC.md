# OTA — 空中升级

[← 返回技术栈](../MOC.md) | [← 主页](../../index.md)|[mcu内存分配](https://app.diagrams.net/#Hevil0knight/Basic_learning_record/main/术中自有万钟粟/Cortex-M4内核原理/arm_mcu内存划分.drawio)

> Over-the-air:通过无线通信的方式,更新程序和数据

> 这里包括bootloader的知识,根据情况选择性移植内容

---

1. 整体的[流程步骤](流程步骤.md)(或者说OTA的思路), [环境配置](环境配置.md)
2. bootloader阶段:

   1. [芯片烧录模式（ICP / ISP / IAP）与 BOOT 引脚](板子上的boot0和boot1.md)
   2. [IAP实操步骤+原理](IAP实操+原理.md),这里是直接从bootloader跳转到APP,是bootloader基础
   3. [Ymodem传输协议](Ymodem传输协议.md),如何使用Ymodem协议发送文件,并通过串口擦写APP到flash里
   4. [备份+AES加密](AES加密.md),下载先AES加密到一个**备份**区,然后再解密到运行区去运行
