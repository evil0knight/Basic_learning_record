# Flash

[← 返回模块索引](../MOC.md) | [← 主页](../../index.md)

[← 返回 OTA 升级](../../术中自有万钟粟/OTA/MOC.md) |

> [Flash 知识](./知识/MOC.md)

---

> 请求：外部->事件组 →异步task-> 消息队列->flash_handler
> 完成：flash_handler->回调 → 信号量-> 异步task→ 事件组->外部

## 关键文件

- [service_externflash_manage.h](./service_externflash_manage/service_externflash_manage.h) / [service_externflash_manage.c](./service_externflash_manage/service_externflash_manage.c)：
  这里有个storage_manager_**task**(void *argument),在C文件中,通过**事件组的方式**去通知**异步擦写flashtask**
  还有暴露一些操控这个任务的函数,他是最顶层的接口,其他的地方想控制flash就调用这里的函数
- [externflash_manage_port.h](./service_externflash_manage/externflash_manage_port.h) / [externflash_manage_port.c](./service_externflash_manage/externflash_manage_port.c)：
  这里完全就是调用wrapper层提供上来的,这里是业务层的一层解耦,
  负责写异步擦写flash的service代码的人员只从这里看函数.
- [bsp_wrapper_flash.h](./bsp_wrapper_flash/bsp_wrapper_flash.h) / [bsp_wrapper_flash.c](./bsp_wrapper_flash/bsp_wrapper_flash.c)：
  这里就是上层开发者使用下层服务的基层,把他当成一个对象实例,直接是面向对象的思想
  再做一层封装,检查合法性,防止adapter更换
  对外提供方便的函数使用,把集成好的结构体通过函数,安全的开放
- [bsp_adapter_port_flash.h](./bsp_adapter_port_flash/bsp_adapter_port_flash.h) / [bsp_adapter_port_flash.c](./bsp_adapter_port_flash/bsp_adapter_port_flash.c)：
  输入做一层薄封装,集成handler资源并封装成一个操作结构体,想供wrapper实例调用
  在 `bsp_adapter_flash_register`去把底层的资源,挂载在这个操作结构体上
  wrapper向他提供register函数,他在 `bsp_adapter_flash_register`最底下去使用 `wrapper_register`函数防止上下层耦合
  然后这个操作函数提供给wrapper层的实例去使用
  这里会挂载所有的函数,包括event的回调函数
- [bsp_flash_handler.h](./bsp_storage/handler/Inc/bsp_flash_handler.h) / [bsp_flash_handler.c](./bsp_storage/handler/Src/bsp_flash_handler.c)：
  event里包含了回调函数指针,底层回调的时候可以直接借助event回调,
  flash_handler_event_put这里给外部使用,发送队列事件给handler
  flash_event_handle这里在handler的task里根据事件,执行具体driver
  flash_handler_thread这里使用argue挂载资源,注册handler实例,等待队列消息去使用driver
  使用好了之后,直接使用event里传过来的指针执行回调函数read_finish_callback,里面释放信号量提醒上层
- [bsp_w25q64_driver.h](./bsp_storage/driver/W25Q64/Inc/bsp_w25q64_driver.h) / [bsp_w25q64_driver.c](./bsp_storage/driver/W25Q64/Src/bsp_w25q64_driver.c)：
  这里写具体的操作函数,然后需要什么资源要从adapter注册到handler,
  再通过handler初始化的时候,用driver向上提供的w25q64_inst注册进来
- [bsp_chipflash_driver.h](./bsp_storage/driver/STM32F411/bsp_chipflash_driver.h) / [bsp_chipflash_driver.c](./bsp_storage/driver/STM32F411/bsp_chipflash_driver.c)：STM32F411 片内 Flash Driver 接口与实现。
