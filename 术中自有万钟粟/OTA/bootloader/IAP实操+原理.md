# IAP 实操 + 原理

[← bootloader](./MOC.md) | [← 主页](../../../index.md)

---

## 原理:

在keil里配置程序flash0地址和4地址里的MSP和跳转函数,先跳转到用户bootloader里,然后用户bootloader再跳转到app里

## 实操

[环境配置](环境配置.md)后,
