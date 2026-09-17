#include "adc_core.h"

#include "user_task_reso_config.h"

/*
 * ADC 双缓冲与任务实现
 *
 * 事件走任务通知，互斥量只做互斥，两者分工：
 *   中断 → 搬运任务：ADC_BIT_DONE，本轮搬运完成
 *   搬运任务 → 采样任务：ADC_PRODUCED(x)，第 x 块已写好
 *   采样任务 → 搬运任务：ADC_CONSUMING(x)，第 x 块正在被消费
 *
 * 产生位和消费位在各自任务的通知值里编号相同，采样任务处理完一块
 * 再清掉它自己的产生位；搬运任务看到下一块还有产生位，就说明那一块
 * 还没被消费完，要等消费位，再用互斥量等采样任务处理完。
 */

#define ADC_BIT(x)       (1UL << (x))
#define ADC_BIT_DONE     ADC_BIT(8)
#define ADC_PRODUCED(x)  ADC_BIT(x)
#define ADC_CONSUMING(x) ADC_BIT(x)

static uint16_t s_adc_buffer[2][ADC_CORE_BUFFER_SIZE];

/* 正在被外设写入的块下标；另一块就是产生位标记的待处理块 */
static volatile uint8_t s_adc_write_index;

static osal_task_handle_t s_adc_transfer_task = NULL;
static osal_task_handle_t s_adc_sample_task = NULL;
static osal_mutex_handle_t s_adc_mutex = NULL;

/*
 * 转换完成回调在中断里执行：只通知搬运任务，
 * Flash、日志和其他耗时操作都放到任务里。
 */
static void adc_core_conv_callback(en_core_adc_instance_t instance,
                                   uint16_t *p_buffer,
                                   uint16_t count)
{
    (void)instance;
    (void)p_buffer;
    (void)count;

    (void)osal_task_notify_from_isr(s_adc_transfer_task, ADC_BIT_DONE);
}

osal_status_t adc_core_init(void)
{
    s_adc_transfer_task = user_task_get(USER_TASK_IDX_ADC_TRANSFER);
    s_adc_sample_task = user_task_get(USER_TASK_IDX_ADC_SAMPLE);
    s_adc_mutex = user_mutex_get(USER_MUTEX_IDX_ADC);

    if ((s_adc_transfer_task == NULL) || (s_adc_sample_task == NULL) ||
        (s_adc_mutex == NULL))
    {
        return OSAL_ERR_INVALID_ARGUMENT;
    }

    s_adc_write_index = 0U;

    if (core_adc_init() != CORE_ADC_OK)
    {
        return OSAL_ERROR;
    }

    return OSAL_SUCCESS;
}

osal_status_t adc_core_start(void)
{
    if (core_adc_register_conv_callback(CORE_ADC1,
                                       adc_core_conv_callback) != CORE_ADC_OK)
    {
        return OSAL_ERROR;
    }

    if (core_adc_start(CORE_ADC1, s_adc_buffer[s_adc_write_index],
                       ADC_CORE_BUFFER_SIZE) != CORE_ADC_OK)
    {
        return OSAL_ERROR;
    }

    return OSAL_SUCCESS;
}

void adc_core_stop(void)
{
    (void)core_adc_stop(CORE_ADC1);
}

void adc_transfer_task(void *argument)
{
    uint32_t notify_value = 0U;

    (void)argument;

    for (;;)
    {
        /* 1. 等本轮搬运完成，退出时清掉完成位 */
        do
        {
            (void)osal_task_notify_wait(0U, ADC_BIT_DONE, &notify_value,
                                        OSAL_WAIT_FOREVER);
        } while (0U == (notify_value & ADC_BIT_DONE));

        /* 2. 告诉采样任务：当前写入块已经写好，并取回它的产生位 */
        if (osal_task_notify_and_query(
                s_adc_sample_task, ADC_PRODUCED(s_adc_write_index),
                &notify_value) != OSAL_SUCCESS)
        {
            notify_value = 0U;
        }

        /* 3. 换写入块，原来那一块由产生位标记为待处理 */
        s_adc_write_index = (uint8_t)(1U - s_adc_write_index);

        /* 4. 新写入块还没被消费完就等采样任务 */
        if ((notify_value & ADC_PRODUCED(s_adc_write_index)) != 0U)
        {
            /* 4.1 等消费通知 */
            do
            {
                (void)osal_task_notify_wait(
                    0U, ADC_CONSUMING(s_adc_write_index), &notify_value,
                    OSAL_WAIT_FOREVER);
            } while (0U == (notify_value &
                            ADC_CONSUMING(s_adc_write_index)));

            /* 4.2 再用互斥量等采样任务真的处理完 */
            if (osal_mutex_take(s_adc_mutex, OSAL_WAIT_FOREVER)
                == OSAL_SUCCESS)
            {
                (void)osal_mutex_give(s_adc_mutex);
            }
        }
        else
        {
            /* 4.3 已经消费完，清掉残留的消费位 */
            (void)osal_task_notify_value_clear(
                ADC_CONSUMING(s_adc_write_index));
        }

        /* 5. 启动下一轮搬运 */
        (void)core_adc_start(CORE_ADC1, s_adc_buffer[s_adc_write_index],
                             ADC_CORE_BUFFER_SIZE);
    }
}

void adc_sample_task(void *argument)
{
    uint32_t notify_value = 0U;
    uint32_t index;

    (void)argument;

    for (;;)
    {
        /* 1. 等产生通知，不在进入和退出时清位 */
        (void)osal_task_notify_wait(0U, 0U, &notify_value,
                                    OSAL_WAIT_FOREVER);

        /* 2. 逐块处理 */
        for (index = 0U; index < 2U; index++)
        {
            if ((notify_value & ADC_PRODUCED(index)) == 0U)
            {
                continue;
            }

            if (osal_mutex_take(s_adc_mutex, OSAL_WAIT_FOREVER)
                != OSAL_SUCCESS)
            {
                continue;
            }

            /* 2.1 先声明这块正在被消费，再处理 */
            (void)osal_task_notify(s_adc_transfer_task,
                                   ADC_CONSUMING(index));

            /* 2.2 消费数据 */
            adc_user_process(s_adc_buffer[index], ADC_CORE_BUFFER_SIZE);

            /* 2.3 处理完清产生位，再放锁 */
            (void)osal_task_notify_value_clear(ADC_PRODUCED(index));
            (void)osal_mutex_give(s_adc_mutex);
        }
    }
}

/* 业务处理入口：工程自己实现，默认空实现 */
__WEAK void adc_user_process(uint16_t *p_buffer, uint16_t count)
{
    (void)p_buffer;
    (void)count;
}
