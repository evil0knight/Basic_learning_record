# platform_bsp

[返回 firmware](../../MOC.md) | [返回代码架构](../../../../MOC.md) | [返回主页](../../../../../index.md)

> 2026/10/28,wraper实例,adapter挂载,向下argument注入

## 关键文件

- [bsp_wrapper_xxx.h](./bsp_wrapper/bsp_wrapper_xxx.h) / [bsp_wrapper_xxx.c](./bsp_wrapper/bsp_wrapper_xxx.c)：
  面向上层提供稳定的 XXX 设备接口，完成参数检查、状态转换和对外操作；关键函数为 `bsp_wrapper_xxx_register()`、`bsp_wrapper_xxx_init()` 和 `bsp_wrapper_xxx_control()`。
  这里就是上层开发者使用下层服务的基层,把他当成一个对象实例,直接是面向对象的思想
  再做一层封装,检查合法性,防止adapter更换
  对外提供方便的函数使用,把集成好的结构体通过函数安全地开放
- [bsp_adapter_port_xxx.h](./bsp_adapter/bsp_adapter_port_xxx.h) / [bsp_adapter_port_xxx.c](./bsp_adapter/bsp_adapter_port_xxx.c)：
  分为两个部分,向下的argument和向上的register

  1. register:上层人员思维:器件序号+操作

     wrapper会最终决定对象是什么样的,wrapper.h会定义一个实例结构体,
     然后实例化是wrapper自己先实例化一个,adapter层包含wrapper.h调用结构体在adapter层的注册函数里短暂实例化,把adapter的函数挂上去然后传到wrapper里,传完之后函数结束就销毁了

     从顶往下看,这个实例需要什么函数,你就在adapter层写个空壳,然后挂上去
     空壳里填写的是就是handler或者driver(无handler)的操作函数,包含了handler和driver.h就能用

     所以adapter函数里上面是很多的函数声明,底下函数实现,然后再加一个register底下再调用wrapper的reg去传参
  2. 有task:用argument:下层人员思维:OS资源,mcu资源注入例如这样的argument

     ```
     static ahtxx_yield_interface_t g_yieldInterface = {
         .pf_rtos_yield = osal_task_delay_ms
     };
     static ahtxx_timebase_interface_t g_timebaseInterface = {
         .pf_get_tick_count = HAL_GetTick
     };
     static temp_humi_handler_os_interface_t g_osInterface = {
         .os_delay_ms = osal_task_delay_ms,
         .os_queue_create = (temp_humi_status_t (*)(uint32_t const, uint32_t const, void ** const))osal_queue_create,
         .os_queue_put = (temp_humi_status_t (*)(void * const, void * const, uint32_t))osal_queue_send,
         .os_queue_get = (temp_humi_status_t (*)(void *, void *, uint32_t))osal_queue_receive
     };

     temp_humi_handler_all_input_arg_t input_arg = {
         .iic_driver_interface = &g_iicDriverInterface,
         .timebase_interface = &g_timebaseInterface,
         .os_interface = &g_osInterface,
         .yield_interface = &g_yieldInterface
     };
     ```
     下层是需要资源的,所以通过一个argument把资源通过task的形参传递到底层的handler或者driver实例去
  3. 没有task:直接在实例的init里调用底层的inst,去挂载资源,再加上注册回调函数例如

     ```
     static bsp_st7789_driver_t g_st7789DriverInstance;
     static basic_oper_driver_interface_t g_st7789OperInstance = {
         .pf_spi_transmit = spi_transmit_data,
         .pf_spi_transmit_dma = spi_transmit_data_dma,
         .pf_write_cs_pin = gpio_write_cs_pin,
         .pf_write_dc_pin = gpio_write_dc_pin,
         .pf_write_reset_pin = gpio_write_reset_pin
     };
     static st7789_spi_driver_interface_t g_st7789SpiInstance;
     static st7789_timebase_interface_t g_st7789TimeInstance = {
         .pf_delay_no_os = osal_task_delay_ms,
         .pf_get_tick_count = osal_task_get_tick_count
     };
     static st7789_yield_interface_t g_st7789YieldInterface = {
         .pf_rtos_yield = osal_task_delay_ms
     };
     static osal_sema_handle_t g_displayDmaDoneSema = NULL;
     static volatile bool g_displayDmaActive = false;
     static volatile bool g_displayDmaError = false;


     static void display_drv_init(display_drv_t *dev)
     {
         (void)dev;

         if (g_displayDmaDoneSema == NULL)
         {
             (void)osal_sema_binary_create(&g_displayDmaDoneSema);
             spi_drain_dma_done_signal();
         }

         st7789_instance_driver_link(&g_st7789DriverInstance,
                                     &g_st7789OperInstance,
                                     &g_st7789SpiInstance,
                                     &g_st7789TimeInstance,
                                     &g_st7789YieldInterface);
         g_st7789DriverInstance.pf_st7789_init(&g_st7789DriverInstance);
     }
     ```
