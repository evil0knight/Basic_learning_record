/*****************************************************************************
 * Copyright (C) 2026 看门狗模块贡献者。
 *
 * 保留所有权利。
 *
 * @file watchdog_adapter_port.c
 *
 * @par dependencies
 * - watchdog_adapter_port.h
 * - stm32f4xx_hal.h
 *
 * @author YA
 *
 * @brief 提供硬件看门狗、备份计数和系统复位适配接口。
 *
 * 处理流程：
 * 由上层初始化并调用接口，具体操作见各函数说明。
 *
 * @version V1.0
 *
 * @note 1 个制表符等于 4 个空格；注释采用 UTF-8 中文。
 * @note LSI 为 32 kHz 时超时约 5 秒，实际时间受 LSI 频率影响。
 * @note 备份计数仅在备份域持续供电且未被复位时保留。
 *
 ****************************************************************************/

//****************************** Includes begin ******************************//
#include "watchdog_adapter_port.h"
#include "stm32f4xx_hal.h"
//******************************* Includes end *******************************//

//************************** Static Variables begin **************************//
static IWDG_HandleTypeDef s_hiwdg;
//*************************** Static Variables end ***************************//

//************************ Function Definitions begin ************************//

/**
 * @brief 初始化 STM32F411 独立看门狗和备份域访问。
 *
 * 步骤：
 *  1. 使能 PWR 时钟，开放备份域写访问。
 *  2. 配置 IWDG 为 64 分频、重装值 2500，调用 HAL 初始化。
 *
 * @param 无。
 *
 * @return 无；当前实现未处理 HAL 初始化失败。
 *
 */
void watchdog_port_init(void)
{

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    s_hiwdg.Instance = IWDG;
    s_hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    s_hiwdg.Init.Reload = 2500U;
    if (HAL_IWDG_Init(&s_hiwdg) != HAL_OK)
    {

    }
}

/**
 * @brief 刷新独立看门狗计数器。
 *
 * 步骤：
 *  1. 调用 HAL 刷新已初始化的 IWDG。
 *
 * @param 无。
 *
 * @return 无；当前实现未检查 HAL 返回值。
 *
 */
void watchdog_port_refresh(void)
{
    HAL_IWDG_Refresh(&s_hiwdg);
}

/**
 * @brief 读取保存的启动失败计数。
 *
 * 步骤：
 *  1. 读取 RTC 备份寄存器 BKP0R。
 *
 * @param 无。
 *
 * @return uint32_t：已保存的启动失败计数。
 *
 */
uint32_t watchdog_port_boot_fail_read(void)
{

    return RTC->BKP0R;
}

/**
 * @brief 保存启动失败计数。
 *
 * 步骤：
 *  1. 开放备份域写访问，再写入 RTC 备份寄存器 BKP0R。
 *
 * @param[in] count : 待保存的启动失败计数。
 *
 * @return 无。
 *
 */
void watchdog_port_boot_fail_write(uint32_t count)
{

    PWR->CR |= PWR_CR_DBP;
    RTC->BKP0R = count;
}

/**
 * @brief 判断本次启动是否带有 IWDG 复位标志。
 *
 * 步骤：
 *  1. 读取 IWDG 复位标志。
 *  2. 清除全部 RCC 复位标志，再返回读取结果。
 *
 * @param 无。
 *
 * @return bool：true 表示存在 IWDG 复位标志，否则为 false。
 *
 */
bool watchdog_port_is_wdg_reset(void)
{
    bool is_wdg_reset = ((RCC->CSR & RCC_CSR_IWDGRSTF) != 0U);

    __HAL_RCC_CLEAR_RESET_FLAGS();

    return is_wdg_reset;
}

/**
 * @brief 请求系统软件复位。
 *
 * 步骤：
 *  1. 调用 CMSIS 系统复位接口。
 *
 * @param 无。
 *
 * @return 无；正常执行复位后不返回。
 *
 */
void watchdog_port_system_reset(void)
{
    NVIC_SystemReset();
}

//************************* Function Definitions end *************************//

//******************************* End of File ********************************//
