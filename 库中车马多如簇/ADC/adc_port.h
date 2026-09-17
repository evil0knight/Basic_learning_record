#ifndef ADC_PORT_H
#define ADC_PORT_H

#include <stdint.h>

#include "main.h"
#include "adc.h"

/*
 * ADC 平台抽象层（platform_mcu 层 Port）
 *
 * 统一 core_adc_* 接口，屏蔽 HAL 细节：轮询单次测量、单次搬运启动、
 * 转换完成与错误回调注册。DMA 与 ADC 转换完成中断共用同一套回调桥，
 * 上层任务不感知底层搬运方式。静态表在 adc_port_config.h 配置。
 */

/* ADC 状态码 */
typedef enum
{
    CORE_ADC_OK = 0,
    CORE_ADC_ERROR,
    CORE_ADC_BUSY
} en_core_adc_status_t;

/* ADC 实例枚举 */
typedef enum
{
    CORE_ADC1 = 0,
    CORE_ADC_MAX
} en_core_adc_instance_t;

/* 单实例配置（在 adc_port_config.h 静态表里填） */
typedef struct
{
    ADC_HandleTypeDef *handle;
    uint32_t timeout;
} st_adc_config_t;

/* 转换完成回调：p_buffer=本次数据首地址，count=本次采样点数 */
typedef void (*adc_conv_callback_t)(en_core_adc_instance_t instance,
                                    uint16_t *p_buffer,
                                    uint16_t count);

/* 错误回调：error_code = HAL 错误码 */
typedef void (*adc_error_callback_t)(en_core_adc_instance_t instance,
                                     uint32_t error_code);

#define CORE_ADC_DEFAULT_TIMEOUT 10U

/* 初始化：清回调表和缓冲记录，句柄由 CubeMX 的 MX_ADCx_Init 初始化 */
en_core_adc_status_t core_adc_init(void);

/* 轮询单次测量：启动、等转换完成、读值、停止 */
en_core_adc_status_t core_adc_measure(en_core_adc_instance_t instance,
                                      uint16_t *p_value);

/* 启动单次搬运：DMA 模式走 DMA，否则走转换完成中断 */
en_core_adc_status_t core_adc_start(en_core_adc_instance_t instance,
                                    uint16_t *p_buffer,
                                    uint16_t count);

/* 停止搬运 */
en_core_adc_status_t core_adc_stop(en_core_adc_instance_t instance);

/* 注册转换完成回调 */
en_core_adc_status_t core_adc_register_conv_callback(
    en_core_adc_instance_t instance, adc_conv_callback_t callback);

/* 注册错误回调 */
en_core_adc_status_t core_adc_register_error_callback(
    en_core_adc_instance_t instance, adc_error_callback_t callback);

/* 取 HAL 句柄（特殊场景直接操作 HAL 用） */
ADC_HandleTypeDef *core_adc_get_handle(en_core_adc_instance_t instance);

#endif /* ADC_PORT_H */
