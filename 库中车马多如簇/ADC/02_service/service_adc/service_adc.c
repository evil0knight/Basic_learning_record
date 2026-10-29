/******************************************************************************
 * Copyright (C) 2026 XXXXXXXXXXX, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file service_adc.c
 *
 * @par dependencies
 * - service_adc.h
 * - user_task_reso_config.h
 *
 * @author YA
 *
 * @brief ADC 双缓冲、任务通知和采样处理服务。
 *
 * Processing flow:
 *
 * 完成通知 -> 切换缓冲 -> 启动下一轮；就绪通知 -> 业务处理。
 *
 * @version V1.0
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//****************************** Includes begin ******************************//
#include "service_adc.h"

#include "user_task_reso_config.h"
//******************************* Includes end *******************************//

//****************************** Defines begin *******************************//
/* 完成位属于搬运任务；产生位属于采样任务，消费位属于搬运任务。 */
#define ADC_BIT(x)       (1UL << (x))
#define ADC_BIT_DONE     ADC_BIT(8)
#define ADC_PRODUCED(x)  ADC_BIT(x)
#define ADC_CONSUMING(x) ADC_BIT(x)
//******************************* Defines end ********************************//

//************************** Static Variables begin **************************//
/* 两块缓冲区由 Service 持有，Platform 只使用传入的写入地址。 */
static uint16_t s_adc_buffer[2][SERVICE_ADC_BUFFER_SIZE];

/* 当前交给外设写入的缓冲区下标。 */
static volatile uint8_t s_adc_write_index;

static osal_task_handle_t s_service_adc_transfer_task = NULL;
static osal_task_handle_t s_service_adc_sample_task = NULL;
static osal_mutex_handle_t s_adc_mutex = NULL;
//*************************** Static Variables end ***************************//

//************************** Static Functions begin **************************//
/**
 * @brief 在 ADC 完成中断中通知搬运任务。
 *
 * Steps:
 *  1. 向搬运任务发送 ADC_BIT_DONE 通知位。
 *
 * @param[in] instance : 本次完成的 ADC 实例，当前未使用。
 * @param[in] p_buffer : 本轮采样缓冲区地址，当前未使用。
 * @param[in] count : 本轮记录的采样点数，当前未使用。
 * @return void : 回调无返回值。
 * @note 仅发送完成通知；耗时处理由任务执行。
 *
 */
static void service_adc_conv_callback(en_core_adc_instance_t instance,
                                   uint16_t *p_buffer,
                                   uint16_t count)
{
    (void)instance;
    (void)p_buffer;
    (void)count;

    (void)osal_task_notify_from_isr(s_service_adc_transfer_task, ADC_BIT_DONE);
}

//*************************** Static Functions end ***************************//

//************************ Function Definitions begin ************************//
/**
 * @brief 初始化 ADC 采集服务及双缓冲状态。
 *
 * Steps:
 *  1. 从统一资源表取得两个任务句柄和互斥量。
 *  2. 检查资源并重置双缓冲写入下标。
 *
 * @return osal_status_t : 成功返回 OSAL_SUCCESS，否则返回资源或初始化错误。
 * @note 由 App 先调用 core_adc_init，再在任务创建后初始化本服务。
 *
 */
osal_status_t service_adc_init(void)
{
    s_service_adc_transfer_task = user_task_get(USER_TASK_IDX_ADC_TRANSFER);
    s_service_adc_sample_task = user_task_get(USER_TASK_IDX_ADC_SAMPLE);
    s_adc_mutex = user_mutex_get(USER_MUTEX_IDX_ADC);

    //-------------------------参数检查 begin-------------------------//
    if ((s_service_adc_transfer_task == NULL) ||
        (s_service_adc_sample_task == NULL) ||
        (s_adc_mutex == NULL))
    {
        return OSAL_ERR_INVALID_ARGUMENT;
    }

    //--------------------------参数检查 end--------------------------//

    s_adc_write_index = 0U;

    return OSAL_SUCCESS;
}

/**
 * @brief 注册采集完成回调并启动首轮采集。
 *
 * Steps:
 *  1. 将服务的通知回调注册到 CORE_ADC1。
 *  2. 将当前写入块交给 ADC 平台启动采集。
 *
 * @return osal_status_t : 启动成功返回 OSAL_SUCCESS，否则返回 OSAL_ERROR。
 * @note 先调用 service_adc_init；本函数每次调用都会设置完成回调。
 *
 */
osal_status_t service_adc_start(void)
{
    if (core_adc_register_conv_callback(CORE_ADC1,
        service_adc_conv_callback) != CORE_ADC_OK)
    {
        return OSAL_ERROR;
    }

    if (core_adc_receive_dma(CORE_ADC1, s_adc_buffer[s_adc_write_index],
                       SERVICE_ADC_BUFFER_SIZE) != CORE_ADC_OK)
    {
        return OSAL_ERROR;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 请求停止 CORE_ADC1 的采集。
 *
 * Steps:
 *  1. 调用 ADC 平台停止接口。
 *
 * @return void : 当前接口不向调用者返回停止结果。
 * @note 只停止底层采集，不退出任务或清理待处理通知。
 *
 */
void service_adc_stop(void)
{
    (void)core_adc_stop_dma(CORE_ADC1);
}

/**
 * @brief 等待采集完成、切换写入缓冲区并启动下一轮。
 *
 * Steps:
 *  1. 等待 ADC_BIT_DONE，通知采样任务消费已完成块。
 *  2. 切换写入块，按通知位和互斥量等待该块处理完成。
 *  3. 将新写入块交给 ADC 平台，开始下一轮采集。
 *
 * @param[in] argument : 任务入口参数，当前未使用。
 * @return void : 任务入口持续运行，不正常返回。
 * @note 普通 DMA 完成后由任务重启，两轮采集之间存在间隙。
 *
 */
void service_adc_transfer_task(void *argument)
{
    uint32_t notify_value = 0U;

    (void)argument;

    for (;;)
    {

        do
        {
            (void)osal_task_notify_wait(0U, ADC_BIT_DONE, &notify_value,
                                        OSAL_WAIT_FOREVER);
        } while (0U == (notify_value & ADC_BIT_DONE));

        if (osal_task_notify_and_query(
                s_service_adc_sample_task, ADC_PRODUCED(s_adc_write_index),
                &notify_value) != OSAL_SUCCESS)
        {
            notify_value = 0U;
        }

        /* 完成块交给采样任务，下一轮改用另一块。 */
        s_adc_write_index = (uint8_t)(1U - s_adc_write_index);

        if ((notify_value & ADC_PRODUCED(s_adc_write_index)) != 0U)
        {

            do
            {
                (void)osal_task_notify_wait(
                    0U, ADC_CONSUMING(s_adc_write_index), &notify_value,
                    OSAL_WAIT_FOREVER);
            } while (0U == (notify_value &
                            ADC_CONSUMING(s_adc_write_index)));

            /* 消费通知表示处理已开始，还需等待互斥量释放。 */
            if (osal_mutex_take(s_adc_mutex, OSAL_WAIT_FOREVER)
                == OSAL_SUCCESS)
            {
                (void)osal_mutex_give(s_adc_mutex);
            }
        }
        else
        {

            (void)osal_task_notify_value_clear(
                ADC_CONSUMING(s_adc_write_index));
        }

        (void)core_adc_receive_dma(CORE_ADC1, s_adc_buffer[s_adc_write_index],
                             SERVICE_ADC_BUFFER_SIZE);
    }
}

/**
 * @brief 等待缓冲区就绪并执行采样数据处理。
 *
 * Steps:
 *  1. 等待产生通知，依次检查两块缓冲区。
 *  2. 取得互斥量，发送消费通知并调用业务处理入口。
 *  3. 清除本块产生位并释放互斥量。
 *
 * @param[in] argument : 任务入口参数，当前未使用。
 * @return void : 任务入口持续运行，不正常返回。
 * @note 业务处理在任务上下文执行，处理期间持有 ADC 互斥量。
 *
 */
void service_adc_sample_task(void *argument)
{
    uint32_t notify_value = 0U;
    uint32_t index;

    (void)argument;

    for (;;)
    {

        (void)osal_task_notify_wait(0U, 0U, &notify_value,
                                    OSAL_WAIT_FOREVER);

        for (index = 0U; index < 2U; index++)
        {
            if ((notify_value & ADC_PRODUCED(index)) == 0U)
            {
                continue;
            }

            /* 消费通知表示处理已开始，还需等待互斥量释放。 */
            if (osal_mutex_take(s_adc_mutex, OSAL_WAIT_FOREVER)
                != OSAL_SUCCESS)
            {
                continue;
            }

            (void)osal_task_notify(s_service_adc_transfer_task,
                                   ADC_CONSUMING(index));

            /* 处理期间持锁，返回后清除产生位并释放该块。 */
            service_adc_user_process(s_adc_buffer[index],
                                     SERVICE_ADC_BUFFER_SIZE);

            (void)osal_task_notify_value_clear(ADC_PRODUCED(index));
            (void)osal_mutex_give(s_adc_mutex);
        }
    }
}

/**
 * @brief 处理一块已完成采集的数据。
 *
 * Steps:
 *  1. 由采样任务调用，默认弱实现不处理数据。
 *
 * @param[in] p_buffer : 已完成缓冲区地址，处理期间由采样任务持锁。
 * @param[in] count : 本块采样点数。
 * @return void : 业务处理入口无返回值。
 * @note 工程可提供同名强定义完成滤波、换算或上报；返回后不再占用缓冲区。
 *
 */
__WEAK void service_adc_user_process(uint16_t *p_buffer, uint16_t count)
{
    (void)p_buffer;
    (void)count;
}

//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
