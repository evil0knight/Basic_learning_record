# DMA：

[← 返回 Cortex-M4内核原理](./MOC.md) | [← 主页](../../index.md)

---

## 原理:

1. [更好的阅读体验](https://app.diagrams.net/#Hevil0knight/Basic_learning_record/main/术中自有万钟粟/Cortex-M4内核原理/arm_mcu内存划分.drawio):这里有关于总线部分的内容
2. 通过总线矩阵的不同路径,避免和cpu公用同一条AHB
3. 通过FIFO减少并行时SBUS和DMA仲裁导致丢数据,FIFO是一个DMA的缓存,可以等待SBUS使用完总线,
   在此期间先写到FIFO里,然后又SBUS使用完再一次性写入,减少总线仲裁
   如果没有FIFO,就有可能发生溢出错误,因为DMA得不到写入机会,下一个数据又已经完成,就会覆盖

## 操作

- [DMA操作外设GPIO](../嵌软高手/DMA操作外设GPIO.md)
