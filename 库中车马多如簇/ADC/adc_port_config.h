#ifndef ADC_PORT_CONFIG_H
#define ADC_PORT_CONFIG_H

#include "adc_port.h"

/*
 * ADC 槽位表：每个实例填 HAL 句柄和轮询测量超时。
 * 移植时用目标板资源替换表项。
 */

/* 1 走 DMA 搬运，0 走 ADC 转换完成中断搬运 */
#define ADC_PORT_USE_DMA 1U

static const st_adc_config_t g_adc_configs[CORE_ADC_MAX] =
{
    [CORE_ADC1] = {
        .handle = &hadc1,
        .timeout = CORE_ADC_DEFAULT_TIMEOUT
    }
};

#endif /* ADC_PORT_CONFIG_H */
