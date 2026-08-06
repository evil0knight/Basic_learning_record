# BSP+Platform

[← 返回嵌入式工程架构](./MOC.md) | [← 主页](../../index.md)

> 就是BSP层

> 参考代码:[BSP_LED](../../库中车马多如簇/LED/BSP_LED/MOC.md)

---


## Driver层--bsp

面向对象思想:

1. 定义对象结构体:包括用uint32_t定义的参数和许多的函数指针,并且还可以进一步的封装,定义一个结构体,里面放函数指针指向特定的函数,然后再在对象中定义一个指向结构体的指针
2. 然后就是定义inst函数(实例化),这个函数的作用是把对象中的函数指针,指向要用的底层函数,实现硬件无关
3. 然后再写driver_xxx_control函数,参数就一个指针,直接指向那个对象,然后通过函数指针直接调用操作函数就好
4. 上面的所有函数,都要注意要先检查参数合法性,定义一个XXX_DEBUG_MODE,可以dubug输出调用状态

## Handler层--bsp

1. 先定义对象结构体,要包括队列(creat,delete,put,get),互斥量(p,v),线程(create,delete),handler_xxx_control,OS_delay等等handler需要用的函数的结构体的指针
2. 然后写inst,传参,连接指针
3. 写register,把driver挂载到handler上进行统一管理,注意handler对象里会定义个driver_group专门储存handler,然后挂载的时候传index上去,就能记住driver的实例,如果有注册数量限制,还要有互斥量保护临界区driver_group
4. 写handler_xxx_control,控制driver,用队列传输msg,然后一个线程监听队列,弹出msg,执行driver_xxx_control

## Platform Adapter层--platform

Adapter 是具体工程与通用 BSP 之间的挂载层，负责把硬件、OS 和 BSP 对象连接起来。

1. Adapter 对外只保留一个 `register` 入口，例如 `drv_adapter_xxx_register()`这个函数后面会在外设统一初始化(`app_perirh_init()`)里调用
2. 在 `register` 内组装 Wrapper 所需的函数表，把 Adapter 内部的初始化、控制桥接函数指针交给 Wrapper 保存。
3. Adapter 内部完成 HAL GPIO、时基、延时、队列、临界区、线程等平台接口的实现，并在初始化桥接函数中依次实例化 Handler 和 Driver。
4. 多实例注册时可以复用一个临时 `index` 变量：`handler_register()` 每次都会把 Driver 放入下一个空闲的 `driver_group[index]`；不要把同一个 index 变量当成所有实例的永久索引保存。

## Wrapper层--platform

Wrapper 是 APP 的稳定外观层，隔离 APP 与 Handler、Driver 以及具体芯片/OS 的接口变化。

1. Wrapper 定义 APP 自己的参数类型和函数表，不把 BSP 的结构体、枚举和平台头文件暴露到 APP。
2. Wrapper 保存 Adapter 注册进来的函数指针，并在 `init`、`control` 等公开 API 中转调这些函数指针；Adapter 不主动调用运行期控制接口。
3. 对多实例设备，APP 传入稳定的逻辑 index，Wrapper/Adapter 将其转换为 Handler 的 `driver_group[index]` 位置；只要注册顺序固定，就不需要再创建第二套 index 映射表。
4. 推荐调用顺序：平台初始化阶段调用 Adapter `register`，系统初始化阶段调用 Wrapper `init`，业务代码只调用 Wrapper 的控制 API。
