# DMA：

[← 返回 Cortex-M4内核原理](./MOC.md) | [← 主页](../../index.md)

---

## 原理:

1. [更好的阅读体验](https://app.diagrams.net/#Hevil0knight/Basic_learning_record/main/术中自有万钟粟/Cortex-M4内核原理/arm_mcu内存划分.drawio):这里有关于总线部分的内容
2. 通过[总线矩阵](总线矩阵.md)的不同路径,避免和cpu公用同一条AHB
3. 通过**FIFO**减少并行时SBUS和DMA仲裁导致丢数据,FIFO是一个DMA的缓存,可以等待SBUS使用完总线,
   在此期间先写到FIFO里,然后又SBUS使用完再一次性写入,减少总线仲裁
   如果没有FIFO,就有可能发生溢出错误,因为DMA得不到写入机会,下一个数据又已经完成,就会覆盖
   注意FIFO的读写不是并行的
4. 通过**stream流**,让peripheral port和memory port之间有不同的FIFO可以用(可以看[总线矩阵](总线矩阵.md)的第3张图)
5. 每个stream只能选一个**channel**,每个channel对应对应不同外设(可以看[总线矩阵](总线矩阵.md)的第4张图)
6. 如果stream同时来,会有**仲裁器**,判断软件设置的优先级,如果优先级一样就看谁的数字小谁先
7. Burst+FIFO,每次搬运多个字节,且搬运时占用总线(SBUS无法打断),传输占用总线次数更少
   实现AHB更快运行,防止外设溢出,宽位填冲(内存对齐)

## 操作

- [DMA操作外设GPIO](../嵌软高手/DMA操作外设GPIO.md)
- [UART Port](../../库中车马多如簇/UART/UART_Port/MOC.md)(uart+DMA)r
