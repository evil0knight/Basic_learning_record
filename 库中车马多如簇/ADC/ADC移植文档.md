# ADC 移植文档

[← 返回 MOC](./MOC.md) | [← 主页](../../index.md)

## 移植步骤

1. 将 [adc_port.h](./adc_port.h)、[adc_port.c](./adc_port.c)、
   [adc_port_config.h](./adc_port_config.h)、[adc_core.h](./adc_core.h) 和
   [adc_core.c](./adc_core.c) 加入工程编译。

2. 编辑 [adc_port_config.h](./adc_port_config.h)，填写 ADC 槽位表：

```c
static const st_adc_config_t g_adc_configs[CORE_ADC_MAX] =
{
    [CORE_ADC1] = {
        .handle = &hadc1,
        .timeout = CORE_ADC_DEFAULT_TIMEOUT
    }
};
```

3. 在 CubeMX 中配置 ADC 的 GPIO、通道、扫描顺序、采样时间和触发源，
   并在 NVIC 里打开中断：用 DMA 时打开 ADC 对应 DMA 流的全局中断，
   不用 DMA 时打开 ADCx global interrupt。DMA 模式还要配置 ADC 的
   DMA 请求和数据宽度，传输模式选 Normal（单次），因为每轮搬运都由
   `adc_transfer_task` 重新启动。

4. 在 [adc_port_config.h](./adc_port_config.h) 选择底层搬运方式，
   两种方式上层接口完全相同：

```c
/* DMA 搬运 */
#define ADC_PORT_USE_DMA 1U

/* ADC 转换完成中断搬运 */
/* #define ADC_PORT_USE_DMA 0U */
```

5. 编辑 [rtos_config.h](../代码架构/firmware/03_config/rtos_config.h)，
   添加两个任务的栈大小和优先级：

```c
#define CFG_TASK_ADC_TRANSFER_STACK_SIZE 512U
#define CFG_TASK_ADC_TRANSFER_PRIORITY   5U
#define CFG_TASK_ADC_SAMPLE_STACK_SIZE   512U
#define CFG_TASK_ADC_SAMPLE_PRIORITY     4U
```

6. 编辑 [user_task_reso_config.h](../代码架构/firmware/01_app/app_init/user_task_config/user_task_reso_config.h)，
   在 `user_task_idx_t` 的 `USER_TASK_IDX_MAX` 前申请两个任务索引：

```c
typedef enum
{
    USER_TASK_IDX_XXX = 0,
    USER_TASK_IDX_ADC_TRANSFER,
    USER_TASK_IDX_ADC_SAMPLE,
    USER_TASK_IDX_MAX
} user_task_idx_t;
```

7. 在同一个头文件的 `user_mutex_idx_t` 里申请一个 ADC 互斥量。
   事件走任务通知，通知值是任务自带的，不需要再申请信号量或队列：

```c
typedef enum
{
    USER_MUTEX_IDX_XXX = 0,
    USER_MUTEX_IDX_ADC,
    USER_MUTEX_IDX_MAX
} user_mutex_idx_t;
```

8. 编辑 [user_task_reso_config.c](../代码架构/firmware/01_app/app_init/user_task_config/user_task_reso_config.c)，
   在 `g_user_tasks[]` 里填两个任务，顺序必须与枚举顺序一致：

```c
user_task_config_t g_user_tasks[USER_TASK_IDX_MAX] =
{
    /* 启用, 名称, 入口, 栈大小, 优先级, 参数, 句柄 */
    {true, "AdcTransfer", adc_transfer_task,
     CFG_TASK_ADC_TRANSFER_STACK_SIZE,
     CFG_TASK_ADC_TRANSFER_PRIORITY, NULL, NULL},
    {true, "AdcSample", adc_sample_task,
     CFG_TASK_ADC_SAMPLE_STACK_SIZE,
     CFG_TASK_ADC_SAMPLE_PRIORITY, NULL, NULL}
};
```

9. 在同一个文件的 `g_user_mutexes[]` 里填互斥量：

```c
user_mutex_config_t g_user_mutexes[USER_MUTEX_IDX_MAX] =
{
    /* 启用, 句柄 */
    {true, NULL}
};
```

10. 编辑 [user_init.c](../代码架构/firmware/01_app/app_init/user_init.c) 的
    `user_app_init()`：先申请统一资源、启动任务，再初始化 ADC。
    任务句柄是任务通知的目标，`adc_core_init()` 要用，所以必须放在
    `user_tasks_start()` 后面：

```c
status = user_os_resources_init();
if (status != OSAL_SUCCESS)
{
    return status;
}

status = user_tasks_start();
if (status != OSAL_SUCCESS)
{
    user_os_resources_deinit();
    return status;
}

/* 任务由 user_tasks_start 建好，句柄这时才有效 */
if ((user_task_get(USER_TASK_IDX_ADC_TRANSFER) == NULL) ||
    (user_task_get(USER_TASK_IDX_ADC_SAMPLE) == NULL) ||
    (user_mutex_get(USER_MUTEX_IDX_ADC) == NULL))
{
    user_os_resources_deinit();
    return OSAL_ERR_INVALID_ARGUMENT;
}

status = adc_core_init();
if (status != OSAL_SUCCESS)
{
    return status;
}

status = adc_core_start();
if (status != OSAL_SUCCESS)
{
    return status;
}
```

11. 中断里自己的处理写在 [adc_port.c](./adc_port.c) 的
    `HAL_ADC_ConvCpltCallback()`，中断入口本身不用写。用 DMA 时事件从
    DMA 流中断进 HAL，不用 DMA 时从 ADC 中断进 HAL，两条路径最后都
    落到这个回调上。两种挂法：

    1. 通知式（配双缓冲任务用）：处理函数写在 [adc_core.c](./adc_core.c)，
       中断里只通知搬运任务：

```c
static void adc_core_conv_callback(en_core_adc_instance_t instance,
                                   uint16_t *p_buffer,
                                   uint16_t count)
{
    (void)instance;
    (void)p_buffer;
    (void)count;

    (void)osal_task_notify_from_isr(s_adc_transfer_task, ADC_BIT_DONE);
}
```

    2. 不想要双缓冲任务时，直接改桥体，把事件接给自己的处理：

```c
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    en_core_adc_instance_t instance = core_adc_instance_from_handle(hadc);

    if (instance == CORE_ADC1)
    {
        /* 置标志、通知任务、启动下一轮搬运 */
    }
}
```

    通知位 `ADC_BIT_DONE`、`ADC_PRODUCED(x)`、`ADC_CONSUMING(x)` 定义在
    [adc_core.c](./adc_core.c) 顶部，三个位怎么流转见
    [ADC原理文档.md](./ADC原理文档.md)。回调在中断上下文执行，只允许
    `osal_task_notify_from_isr()` 这类短动作，Flash、日志、滤波换算都
    放到任务里做。`core_adc_start()` / `core_adc_stop()` 里的
    `#if ADC_PORT_USE_DMA` 分支跟着第 4 步的宏自动选
    `HAL_ADC_Start_DMA()` 或 `HAL_ADC_Start_IT()`，也不用改。

12. 实现 [adc_core.h](./adc_core.h) 里的 `adc_user_process()`，
    它由 `adc_sample_task` 取到 `USER_MUTEX_IDX_ADC` 后调用：

```c
void adc_user_process(uint16_t *p_buffer, uint16_t count)
{
    /* 滤波、换算、上报等业务处理 */
}
```

13. 停止采样时调用 [adc_core.c](./adc_core.c) 的 `adc_core_stop()`，
    它会关掉 DMA 或 ADC 中断搬运。

## 初始化顺序

`MX_GPIO_Init` → `MX_DMA_Init` → `MX_ADC1_Init` →
`user_os_resources_init` → `user_tasks_start` → `adc_core_init` →
`adc_core_start` → 启动调度器。
