# volatile 关键字

[← 关键字与语法](./关键字与语法.md) | [← C++ 知识地图](./MOC.md)|[←UART中断](../../库中车马多如簇/UART(内含串口助手安装包)/UART接收程序.md)

---

## 作用

告诉编译器：**该变量可能在程序控制之外被修改**，每次访问都必须从内存读取，禁止编译器对其进行缓存优化（寄存器缓存、指令重排）。

RTOS中典型应用：ISR和任务共享变量（中断中修改flag，任务中轮询读取）、多任务共享全局变量、外设寄存器访问。

---

## 典型使用场景

| 场景                        | 说明                                                                            |
| --------------------------- | ------------------------------------------------------------------------------- |
| 硬件寄存器                  | 外设寄存器的值随时可能被硬件改变                                                |
| 中断服务程序（ISR）共享变量 | 主循环读取、ISR 写入的标志位                                                    |
| 多线程共享变量（有限）      | 防止编译器优化，但**不保证[原子性](./原子性与atomic.md)**，不能替代锁/atomic |
| DMA缓冲区                   | DMA控制器直接修改内存                                                           |
| MMIO 操作                   | Memory-mapped I/O 必须加 volatile                                               |

```c
// 硬件寄存器映射
volatile uint32_t* const UART_SR = (uint32_t*)0x40011000;

// ISR 共享标志
volatile bool data_ready = false;

void ISR_Handler() {
    data_ready = true;  // 中断中写
}

void main_loop() {
    while (!data_ready);  // 主循环轮询，不会被优化掉
}
```

<details>
<summary><h3 style="display:inline">函数int square(volatile int *ptr)能实现预期目标吗？为什么？</h3></summary>

函数 `int square(volatile int *ptr) { return *ptr * *ptr; }` **不能**实现预期目标。

原因：因为 `*ptr` 是 volatile 的，编译器会**两次从内存读取** `*ptr` 的值。如果在两次读取之间该值被中断或其他线程修改了，两次读到的值可能不同，结果就不是一个数的平方。

正确写法：

```
int square(volatile int *ptr) 
{    	int val = *ptr;  // 只读一次，存到局部变量  
	return val * val;
}
```

这道题考的就是对 volatile **每次访问都重新读取内存**语义的理解。

`</details>`
