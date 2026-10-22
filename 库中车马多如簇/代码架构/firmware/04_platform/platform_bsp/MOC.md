# platform_bsp

[← firmware](../../MOC.md) | [← 嵌入式工程架构](../../../../../术中自有万钟粟/嵌入式工程架构/MOC.md) | [← 模块总览](../../../../MOC.md) | [← 主页](../../../../../index.md)

---

- [bsp_wrapper_xxx.h](./bsp_wrapper/bsp_wrapper_xxx.h) / [bsp_wrapper_xxx.c](./bsp_wrapper/bsp_wrapper_xxx.c)：
  这里就是上层开发者使用下层服务的基层,把他当成一个对象实例,直接是面向对象的思想
  再做一层封装,检查合法性,防止adapter更换
  对外提供方便的函数使用,把集成好的结构体通过函数,安全的开放
- [bsp_adapter_port_xxx.h](./bsp_adapter/bsp_adapter_port_xxx.h) / [bsp_adapter_port_xxx.c](./bsp_adapter/bsp_adapter_port_xxx.c)：
  输入做一层薄封装,集成handler资源并封装成一个操作结构体,想供wrapper实例调用
  在 `bsp_adapter_xxx_register`去把底层的资源,挂载在这个操作结构体上
  wrapper向他提供register函数,他在 `bsp_adapter_xxx_register`最底下去使用 `wrapper_register`函数防止上下层耦合
  然后这个操作函数提供给wrapper层的实例去使用
  这里会挂载所有的函数,包括event的回调函数
