# 05_bsp

[返回 frimware_bare](../MOC.md) | [返回代码架构](../../MOC.md) | [返回主页](../../../../index.md)

## 关键文件

- [bsp_xxx_handler.h](./bsp_xxx/handler/bsp_xxx_handler.h) / [bsp_xxx_handler.c](./bsp_xxx/handler/bsp_xxx_handler.c)：
  Handler 是 Driver 和上层调用之间的协调层。它定义 Driver 实例，定义handler实例,此时只差资源
  ,接收 Adapter 传入的资源，负责调用 Driver inst、转发设备操作，并根据设备需要管理事件、队列、缓存和任务。Handler 不直接实现寄存器操作，关键函数为 `bsp_xxx_handler_inst()` 和 `bsp_xxx_handler_control()`。
  有了资源就,adapter层要一层薄封装,然后就可以挂载到wrapper了

  引入 Handler 的一个重要原因，是同一个设备的多个 Driver 可能共用队列、时间基准、延时函数、任务句柄、缓存或事件分发器。这些资源属于设备的协调和业务层，只需要由 Handler 保存和管理，不需要重复挂载到每个 Driver 中。
- [bsp_xxx_driver.h](./bsp_xxx/driver/bsp_xxx_driver.h) / [bsp_xxx_driver.c](./bsp_xxx/driver/bsp_xxx_driver.c)：

  最底层的思路是先写一个结构体，这个结构体对应一个器件，然后在结构体里写函数指针，需要什么操作就写什么操作。操作函数依赖上层提供的资源指针实现，上层资源会在上层完成注册，再挂载到 Driver 的资源指针上。

  Driver 的 `inst()` 函数负责保存上层传入的资源，并把 Driver 自己的操作函数挂载到结构体中。这样 Handler 或 Adapter 只需要通过 Driver 结构体里的函数指针操作器件。
